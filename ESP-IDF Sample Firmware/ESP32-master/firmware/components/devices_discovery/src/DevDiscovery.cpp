// See documentation
// https://confluence.mios.com/display/ELFD/Devices+Detection

#include "dev_dis/DevDiscovery.h"

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

#include "wifi_service2/WifiCommands.h"
#include "dev_dis/Commands.h"
#include "Storage.h"
#include "BinaryFileStorageFactory.h"
#include "StorageSection.h"

#include "DebugUtils.h"

namespace dev_dis {

using namespace std::chrono_literals;

namespace {

constexpr auto unknowDevPrecision{ 30 };

constexpr const auto RegAddrToStateStorKey = "RegAddrToState";
constexpr const auto UnknowAddrToStateStorKey = "UnkAddrToState";
constexpr const auto StateStorKey = "State";

constexpr const auto StateWasChangedSavePeriod = 3min;

Rssi smoothRssiValue( const DDTimePoint& prevTp,
                      const DDTimePoint& currTp,
                      const Rssi& prevRssi,
                      const Rssi& currRssi ) {
    if( prevTp >= currTp ) {
        return prevRssi;
    }

    const auto deltaTp = currTp - prevTp;
    constexpr const auto smoothPeriod = 10s;
    if( smoothPeriod <= deltaTp ) {
        return currRssi;
    }

    const auto mult = float( deltaTp.count() ) / float( decltype( deltaTp )( smoothPeriod ).count() );
    const auto addon = float( currRssi - prevRssi ) * mult;
    return prevRssi + addon;
}

}  // namespace

bool DevDiscovery::initializeImpl() {
    if( load() ) {
        m_startCounter = m_uuidToInfo.size();
        if( 0 != m_startCounter ) {
            if( !network::wifi_srv2::startWifiSniffer() ) {
                return false;
            }
        }
    }
    else {
        FUNC_LOGE( "Can't load DevDiscovery, reset it" );
        reset();
    }

    return ParentType::initializeImpl() && bus::Bus::get().join( network::wifi_srv2::responce::BUS_NAME, *this );
}

DevDiscovery::ClbkRet DevDiscovery::updateComponent( UpdContext& ctx ) {
    std::lock_guard<decltype( m_mutex )> lockGuard{ m_mutex };

    const auto currTp = DDClockType::now();
    if( 0 != m_unknowStartCounter ) {
        // If the information about the device has not come for a long time,
        // we consider it lost (the lowest rssi value is used)
        auto itr = m_unknowDev.begin();
        const auto end = m_unknowDev.end();
        while( end != itr ) {
            const auto& [addr, info] = *itr;
            const auto& [rssi, tp] = info;
            if( 1min < currTp - tp ) {
                // FUNC_LOGI( "-----> Lost unknow device by timeout: addr=%s", addr.toHexString().c_str() );
                m_unknowDevUpd[addr] = std::make_tuple( Rssi{}, currTp );
                itr = m_unknowDev.erase( itr );
            }
            else {
                ++itr;
            }
        }
    }

    ctx.tryYield();

    {
        // If the information about the device has not come for a long time,
        // we consider it lost (the lowest rssi value is used)
        auto itr = m_addrToState.begin();
        const auto end = m_addrToState.end();
        while( end != itr ) {
            auto& [addr, state] = *itr;
            auto& [rssi, tp] = state;
            if( 3min < currTp - tp && Rssi{} != rssi ) {
                rssi = Rssi{};
                tp = currTp;
                // FUNC_LOGI( "-----> Lost registered device by timeout: addr=%s", addr.toHexString().c_str() );
                const auto itr = m_addrToInfo.find( addr );
                m_uuidToStateUpdNext[itr->second.getId()] = std::make_tuple( Rssi{}, currTp );
            }
            ++itr;
        }
    }

    ctx.tryYield();

    // Don't use this log in production code! For debug only!
    /*if( !m_unknowDevUpd.empty() ) {
        FUNC_LOGI( "_________________ Founded/Changed unknown devices _________________" );
        for( const auto& [addr, info] : m_unknowDevUpd ) {
            const auto& [rssi, tp] = info;
            FUNC_LOGI( "-----> addr=%s, rssi=%d", addr.toHexString().c_str(), int( rssi ) );
        }
        FUNC_LOGI( "-------------------------------------------------------------------\n" );
    }*/

    // Don't use this log in production code! For debug only!
    /*if( !m_uuidToStateUpdNext.empty() ) {
        FUNC_LOGI( "_________________ Founded/Changed registered devices _________________" );
        for( const auto& [uuid, state] : m_uuidToStateUpdNext ) {
            const auto& [rssi, tp] = state;
            FUNC_LOGI( "-----> addr=%s, rssi=%d", uuid.toHexString().c_str(), int( rssi ) );
        }
        FUNC_LOGI( "-------------------------------------------------------------------\n" );
    }*/

    if( !m_unknowDevUpd.empty() || !m_uuidToStateUpdNext.empty() ) {
        m_uuidToStateLast = m_uuidToStateUpdNext;
        send<responce::DiscoveredDevices>(
            responce::BUS_NAME, std::move( m_uuidToStateUpdNext ), std::move( m_unknowDevUpd ) );
    }

    if( m_addrToStateWasChangedTp && StateWasChangedSavePeriod < currTp - *m_addrToStateWasChangedTp ) {
        saveAddrToState();
    }

    ctx.tryYield();

    if( m_unknowAddrToStateWasChangedTp && StateWasChangedSavePeriod < currTp - *m_unknowAddrToStateWasChangedTp ) {
        saveUnknowAddrToState();
    }

    ctx.tryYield();

    return ClbkRet::Continue;
};

void DevDiscovery::onShutdownStart() {
    ParentType::onShutdownStart();

    if( m_addrToStateWasChangedTp ) {
        saveAddrToState();
    }

    if( m_unknowAddrToStateWasChangedTp ) {
        saveUnknowAddrToState();
    }
}

void DevDiscovery::visit( const bus::BusAddr from,
                          const bus::BusAddr to,
                          const network::wifi_srv2::responce::DiscoveredWifiBeacons& cmd ) {
    std::lock_guard<decltype( m_mutex )> lockGuard{ m_mutex };

    const auto currTp = DDClockType::now();
    const auto end = m_addrToInfo.end();
    for( const auto& [discoveredAddr, discoveredInfo] : cmd.addrToInfo() ) {
        auto itr = m_addrToInfo.find( discoveredAddr );
        const auto currRssi = std::get<1>( discoveredInfo );
        if( end != itr ) {
            // Registered (or known) device processing here
            Rssi smoothedRssi;
            const auto& devInfo = itr->second;
            {
                auto& [prevRssi, prevTp] = m_addrToState[discoveredAddr];
                smoothedRssi = smoothRssiValue( prevTp, currTp, prevRssi, currRssi );
                prevRssi = smoothedRssi;
                prevTp = currTp;
                if( !m_addrToStateWasChangedTp ) {
                    m_addrToStateWasChangedTp = std::make_optional( DDClockType::now() );
                }
            }

            auto& [sendedRssi, sendedTp] = m_uuidToStateLast[devInfo.getId()];
            if( devInfo.getPrecision() < std::abs( smoothedRssi - sendedRssi ) ) {
                m_uuidToStateUpdNext[devInfo.getId()] = std::make_tuple( smoothedRssi, currTp );
            }
        }
        else if( 0 != m_unknowStartCounter ) {
            // Unknown device processing here
            auto& [unknowPrevRssi, unknowPrevTp] = m_unknowDev[discoveredAddr];
            unknowPrevTp = currTp;
            if( unknowDevPrecision < std::abs( currRssi - unknowPrevRssi ) ) {
                unknowPrevRssi = currRssi;
                m_unknowDevUpd[discoveredAddr] = std::make_tuple( currRssi, currTp );

                if( !m_unknowAddrToStateWasChangedTp ) {
                    m_unknowAddrToStateWasChangedTp = std::make_optional( DDClockType::now() );
                }
            }
        }
    }
}

bool DevDiscovery::save( const DeviceInfo& deviceInfo ) const {
    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DISCOVERY_DEVICES_INFO );
    if( !stor ) {
        return false;
    }

    common::VectorWStream stream( 1024 );
    return stream.write( deviceInfo ) && stor->setBlob( deviceInfo.getId().toHexString(), stream.buffer() );
}

bool DevDiscovery::saveAddrToState() {
    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DISCOVERY_DEVICES_SETTINGS );
    if( !stor ) {
        return false;
    }

    m_addrToStateWasChangedTp = std::nullopt;

    common::VectorWStream stream( 2048 );
    return stream.write( m_addrToState ) && stor->setBlob( RegAddrToStateStorKey, stream.buffer() );
}

bool DevDiscovery::saveUnknowAddrToState() {
    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DISCOVERY_DEVICES_SETTINGS );
    if( !stor ) {
        return false;
    }

    m_unknowAddrToStateWasChangedTp = std::nullopt;

    common::VectorWStream stream( 2048 );
    return stream.write( m_unknowDev ) && stor->setBlob( UnknowAddrToStateStorKey, stream.buffer() );
}

bool DevDiscovery::load() {
    {
        auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DISCOVERY_DEVICES_INFO );
        if( !stor ) {
            return false;
        }

        const auto end = stor->cend();
        for( auto itr = stor->cbegin(); itr != end; ++itr ) {
            auto uuid = Uuid::createFromHexStr( *itr );
            if( !uuid ) {
                return false;
            }

            auto value = itr.value();
            using BlobType = storage::KeyValueStorage::BlobType;
            if( !value.is_type<BlobType>() || value.get<BlobType>().empty() ) {
                return false;
            };

            auto blob = value.get<BlobType>();
            common::ArrayRStream stream( blob.data(), blob.size() );

            DeviceInfo devInfo;
            if( stream.read( devInfo ) ) {
                const auto& [itr, isSuccess] = m_uuidToInfo.try_emplace( std::move( *uuid ), std::move( devInfo ) );
                if( isSuccess ) {
                    m_addrToInfo.try_emplace( itr->second.getMac(), itr->second );
                }
            }
            else {
                FUNC_LOGE( "Can't read DeviceInfo: %s", ( *itr ).c_str() );
                return false;
            }
        }
    }

    {
        auto stor =
            storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DISCOVERY_DEVICES_SETTINGS );
        if( !stor ) {
            return false;
        }

        if( stor->exists( RegAddrToStateStorKey ) ) {
            storage::KeyValueStorage::BlobType blob;
            if( !stor->getBlob( RegAddrToStateStorKey, blob ) ) {
                return false;
            }
            common::ArrayRStream rstream( blob.data(), blob.size() );
            if( !rstream.read( m_addrToState ) ) {
                return false;
            }
        }

        if( stor->exists( UnknowAddrToStateStorKey ) ) {
            storage::KeyValueStorage::BlobType blob;
            if( !stor->getBlob( UnknowAddrToStateStorKey, blob ) ) {
                return false;
            }
            common::ArrayRStream rstream( blob.data(), blob.size() );
            if( !rstream.read( m_unknowDev ) ) {
                return false;
            }
        }
    }

    return true;
}

void DevDiscovery::reset() {
    m_uuidToInfo.clear();
    m_addrToInfo.clear();

    m_addrToState.clear();
    m_uuidToStateLast.clear();

    m_unknowDev.clear();
    m_unknowDevUpd.clear();

    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DISCOVERY_DEVICES_INFO );
    stor && stor->removeAll();

    stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DISCOVERY_DEVICES_SETTINGS );
    stor && stor->removeAll();
}

Uuid DevDiscovery::addDevice( const char* name, const MacAddr& addr, const Precision precision ) {
    if( m_addrToInfo.end() != m_addrToInfo.find( addr ) ) {
        return false;
    }

    auto uuid = Uuid::generate();
    DeviceInfo info( uuid, name, addr, precision );

    std::lock_guard<decltype( m_mutex )> lockGuard{ m_mutex };
    auto [itr, isSuccess] = m_uuidToInfo.try_emplace( uuid, std::move( info ) );
    if( !isSuccess ) {
        return Uuid{};
    }

    std::tie( std::ignore, isSuccess ) = m_addrToInfo.try_emplace( addr, itr->second );
    if( !isSuccess ) {
        m_uuidToInfo.erase( itr );
        return Uuid{};
    }

    if( !save( itr->second ) ) {
        return Uuid{};
    }

    m_unknowDev.erase( addr );
    m_unknowDevUpd.erase( addr );

    m_addrToStateWasChangedTp = DDClockType::now();

    send<responce::DeviceAdded>( responce::BUS_NAME, itr->second );
    startScanImpl();
    return uuid;
}

bool DevDiscovery::removeDevice( const Uuid& uuid ) {
    std::lock_guard<decltype( m_mutex )> lockGuard{ m_mutex };

    const auto itr = m_uuidToInfo.find( uuid );
    if( m_uuidToInfo.end() == itr ) {
        return false;
    }

    bool isSuccess{ true };

    const auto& addr = itr->second.getMac();
    if( 0 == m_addrToInfo.erase( addr ) ) {
        isSuccess = false;
    }

    if( 0 == m_uuidToInfo.erase( uuid ) ) {
        isSuccess = false;
    }

    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DISCOVERY_DEVICES_INFO );
    if( !stor || !stor->remove( uuid.toHexString() ) ) {
        isSuccess = false;
    }

    if( !isSuccess ) {
        return false;
    }

    m_addrToStateWasChangedTp = DDClockType::now();

    send<responce::DeviceRemoved>( responce::BUS_NAME, uuid );
    stopScanImpl();
    return true;
}

bool DevDiscovery::editDevice( const Uuid& uuid,
                               const std::optional<std::string>& newName,
                               const std::optional<MacAddr>& newAddr,
                               const std::optional<Precision>& newPrecision ) {
    std::lock_guard<decltype( m_mutex )> lockGuard{ m_mutex };

    const auto itr = m_uuidToInfo.find( uuid );
    if( m_uuidToInfo.end() == itr ) {
        return false;
    }

    std::unique_ptr<responce::DeviceEdited> resp;
    {
        auto oldAddr = itr->second.getMac();
        if( newAddr && oldAddr != *newAddr ) {
            bool isSuccess;
            std::tie( std::ignore, isSuccess ) = m_addrToInfo.try_emplace( *newAddr, itr->second );
            if( !isSuccess ) {
                return false;
            }
            itr->second.setMac( *newAddr );
            m_addrToInfo.erase( oldAddr );

            // https://confluence.mios.com/display/ELFD/Devices+Detection#DevicesDetection-hub.detection.device.set
            // If MAC of device was changed then detectedLastTime and current rssi should be cleaned up.
            m_addrToState[*newAddr] = DevState{};
            m_addrToState.erase( oldAddr );

            resp = std::make_unique<responce::DeviceEdited>( uuid, *newAddr );

            m_addrToStateWasChangedTp = DDClockType::now();
        }
        else {
            resp = std::make_unique<responce::DeviceEdited>( uuid, oldAddr );
        }
    }

    if( newName ) {
        itr->second.setName( *newName );
        resp->setName( *newName );
    }

    if( newPrecision ) {
        itr->second.setPrecision( *newPrecision );
        resp->setPrecision( *newPrecision );
    }

    if( !save( itr->second ) ) {
        return false;
    }

    send( responce::BUS_NAME, std::move( resp ) );

    return true;
}

InfoStateList DevDiscovery::getInfoStateList() const {
    std::lock_guard<decltype( m_mutex )> lockGuard{ m_mutex };
    InfoStateList list;
    const auto endItr = m_addrToState.end();
    for( const auto& [uuid, info] : m_uuidToInfo ) {
        const auto itr = m_addrToState.find( info.getMac() );
        if( endItr == itr ) {
            list.emplace_back( info, std::nullopt );
        }
        else {
            list.emplace_back( info, std::make_optional( itr->second ) );
        }
    }
    return std::move( list );
}

std::optional<DeviceInfo> DevDiscovery::getInfo( const Uuid& uuid ) const {
    std::lock_guard<decltype( m_mutex )> lockGuard{ m_mutex };

    const auto itr = m_uuidToInfo.find( uuid );
    return m_uuidToInfo.end() == itr ? std::nullopt : std::make_optional( itr->second );
}

bool DevDiscovery::startScan() {
    std::lock_guard<decltype( m_mutex )> lockGuard{ m_mutex };
    return startScanImpl();
}

bool DevDiscovery::stopScan() {
    std::lock_guard<decltype( m_mutex )> lockGuard{ m_mutex };
    return stopScanImpl();
}

bool DevDiscovery::startScanImpl() {
    if( 0 == m_startCounter && !network::wifi_srv2::startWifiSniffer() ) {
        return false;
    }

    m_startCounter += 1;
    return true;
}

bool DevDiscovery::stopScanImpl() {
    if( 0 == m_startCounter ) {
        return true;
    }
    else if( 1 == m_startCounter && !network::wifi_srv2::stopWifiSniffer() ) {
        return false;
    }

    m_startCounter -= 1;
    return true;
}

bool DevDiscovery::startUnknowDevDetection() {
    std::lock_guard<decltype( m_mutex )> lockGuard{ m_mutex };

    if( 0 == m_unknowStartCounter && !startScanImpl() ) {
        return false;
    }

    m_unknowDev.clear();
    m_unknowDevUpd.clear();
    m_unknowStartCounter += 1;

    return true;
}

bool DevDiscovery::stopUnknowDevDetection() {
    std::lock_guard<decltype( m_mutex )> lockGuard{ m_mutex };

    if( 0 == m_unknowStartCounter ) {
        return true;
    }
    else if( 1 == m_unknowStartCounter && !stopScanImpl() ) {
        return false;
    }

    m_unknowStartCounter -= 1;

    return true;
}

}  // namespace dev_dis

#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
