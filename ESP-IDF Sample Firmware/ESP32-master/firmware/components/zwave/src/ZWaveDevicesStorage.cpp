
#include <ZWaveDevicesStorage.h>
#include <Storage.h>
#include <StorageFile.h>
#include <GatewayUtils.h>
#include <DebugUtils.h>
#include <ZWaveResources.h>
#include <DeviceSettingsManager.h>
#include <GatewayDefs.h>
#include <ZWaveZNet.h>

namespace gateways {
namespace zwave {

DevicesStorage& DevicesStorage::get() {
    static DevicesStorage storage;
    return storage;
}

bool DevicesStorage::writeVersionHeader( storage::File& file ) const {
    return file.write( resources::storage::version::Major ) && file.write( resources::storage::version::Minor );
}

bool DevicesStorage::restoreAndCheckVersion( storage::File& file ) const {
    static_assert( sizeof( resources::storage::version::Major ) == sizeof( resources::storage::version::Minor ),
                   "Expected same size for major and minor versions." );
    auto version = resources::storage::version::Major;

    // We cannot work with different major versions
    auto result = file.read( version );
    result = result && version == resources::storage::version::Major;

    // We can work with same or older minor version
    result = result && file.read( version );
    result = result && version <= resources::storage::version::Minor;

    return result;
}

DevicesStorage::DevicePtr DevicesStorage::restoreDevice( uint8_t deviceId,
                                                         storage::File& file,
                                                         IGateway::AddingDeviceStatus& addingStatus ) {
    if( !restoreAndCheckVersion( file ) ) {
        addingStatus = IGateway::AddingDeviceStatus::NotSupportedVersion;
        return nullptr;
    }

    auto device = std::make_shared<devices::Device>( deviceId,
                                                     &gateways::zwave::ZNet::get().getRequestsManager(),
                                                     &gateways::zwave::ZNet::get(),
                                                     &gateways::zwave::ZNet::get().getReachableCheckTimeoutSettings() );
    if( !device ) {
        FUNC_LOGE( "Cannot create a device" );
        addingStatus = IGateway::AddingDeviceStatus::BrokenFileFormat;
    }
    auto result = device->restoreData( file );  // TODO   duplicating of code ----*/

    if( result ) {
        addingStatus = IGateway::AddingDeviceStatus::OK;
    }
    else {
        FUNC_LOGE( "Cannot read information from device's file(%u)", deviceId );
        addingStatus = IGateway::AddingDeviceStatus::BrokenFileFormat;
        device.reset();
    }

    return device;
}

bool DevicesStorage::addDeviceFile( devices::Device* device ) {
    std::array<char, gateways::DeviceSettingsManager::MaxNameSize> deviceUrl;

    if( !device ) {
        FUNC_LOGE( "Device's info is wrong" );
        return false;
    }

    gateways::utils::fillDevicePath(
        deviceUrl.data(), deviceUrl.size(), toDevicePrefix( GatewayID::ZWave ), device->getId() );

    if( storage::Storage::get().isExist( deviceUrl.data() ) ) {
        auto numbersAttemps = 0;
        do {
            if( storage::Storage::get().remove( deviceUrl.data() ) ) {
                break;
            }
        } while( numbersAttemps++ < 3 );

        if( numbersAttemps >= 3 ) {
            FUNC_LOGE( "Couldn't remove file: %s", deviceUrl.data() );
        }
    }

    storage::File file;
    if( !file.open( deviceUrl.data(), "wb" ) ) {
        FUNC_LOGE( "Cannot open '%s' file for writting new information about device.", deviceUrl.data() );
        return false;
    }
    auto result = writeVersionHeader( file );

    result = result && device->storeData( file );

    if( !result ) {
        FUNC_LOGE( "Couldn't write all information to '%s' file.", deviceUrl.data() );
        file.close();
        if( storage::Storage::get().remove( deviceUrl.data() ) ) {
            FUNC_LOGE( "Couldn't remove file: %s", deviceUrl.data() );
        }
        return false;
    }

    file.close();

    FUNC_LOGD( "All data saved : %s", result ? "true" : "false" );

    return result;
}

bool DevicesStorage::removeDeviceFile( uint8_t uniqueId ) {
    std::array<char, gateways::DeviceSettingsManager::MaxNameSize> deviceUrl;

    gateways::utils::fillDevicePath( deviceUrl.data(), deviceUrl.size(), toDevicePrefix( GatewayID::ZWave ), uniqueId );

    if( storage::Storage::get().isExist( deviceUrl.data() ) ) {
        auto numbersAttemps = 0;
        do {
            if( storage::Storage::get().remove( deviceUrl.data() ) ) {
                return true;
            }
        } while( numbersAttemps++ < 3 );

        FUNC_LOGE( "Couldn't remove file: %s", deviceUrl.data() );

        return false;
    }
    return true;
}

}  // namespace zwave
}  // namespace gateways
