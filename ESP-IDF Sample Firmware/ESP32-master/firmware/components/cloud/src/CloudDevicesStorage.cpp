#ifdef CONFIG_CLOUD_DEVICES

#include <CloudDevicesStorage.h>
#include <CloudDevice.h>
#include <Storage.h>
#include <StorageFile.h>
#include <GatewayUtils.h>
#include <Defs.h>
#include <DebugUtils.h>
#include <DeviceSettingsManager.h>
#include <GatewayDefs.h>
#include <DeviceInfo.h>

namespace gateways {
namespace cloud {

const char* CloudDevicesListVersion = "CloudDevicesListVersion";

const std::uint8_t Major = 7;
const std::uint8_t Minor = 0;
const std::uint8_t HeaderSize = sizeof( Major ) + sizeof( Minor );

DevicesStorage& DevicesStorage::get() {
    static DevicesStorage storage;
    return storage;
}

bool DevicesStorage::writeVersionHeader( storage::File& file, const char version ) const {
    return file.write( version ) && file.write( Minor );
}

bool DevicesStorage::checkVersion( const char aVersion ) {
    char version = 0;
    if( readVersion( version ) ) {
        return version == aVersion + 10;
    }

    return false;
}

bool DevicesStorage::readVersion( char& version ) {
    std::array<char, gateways::DeviceSettingsManager::MaxNameSize> deviceUrl;
    gateways::utils::fillDevicePath(
        deviceUrl.data(), deviceUrl.size(), toDevicePrefix( GatewayID::Cloud ), CloudDevicesListVersion );

    storage::File file;
    if( storage::Storage::get().isExist( deviceUrl.data() ) && file.open( deviceUrl.data(), "rb" ) ) {
        if( !file.read( version ) ) {
            return false;
        }
    }

    return true;
}

bool DevicesStorage::storeVersion( const char version ) {
    std::array<char, gateways::DeviceSettingsManager::MaxNameSize> deviceUrl;
    gateways::utils::fillDevicePath(
        deviceUrl.data(), deviceUrl.size(), toDevicePrefix( GatewayID::Cloud ), CloudDevicesListVersion );

    storage::File file;
    if( !file.open( deviceUrl.data(), "wb" ) ) {
        file.close();
        return false;
    }

    if( !file.write( version ) ) {
        FUNC_LOGE( "Cloud version cant saved" );
        return false;
    }

    return true;
}

bool DevicesStorage::addDeviceFile( std::unique_ptr<gateways::cloud::Device>& device, const char version ) {
    std::array<char, gateways::DeviceSettingsManager::MaxNameSize> deviceUrl;
    gateways::utils::fillDevicePath(
        deviceUrl.data(), deviceUrl.size(), toDevicePrefix( GatewayID::Cloud ), device.get()->getId() );

    if( storage::Storage::get().isExist( deviceUrl.data() ) ) {
        storage::Storage::get().remove( deviceUrl.data() );
    }

    storage::File file;

    if( !file.open( deviceUrl.data(), "wb" ) ) {
        file.close();
        return false;
    }

    auto result = writeVersionHeader( file, version );
    result = result && device.get()->storeData( file );

    if( !result ) {
        file.close();
        storage::Storage::get().remove( deviceUrl.data() );
        return false;
    }

    file.close();

    storeVersion( version );

    return result;
}

bool DevicesStorage::removeDeviceFile( const std::string& uniqueId ) {
    std::array<char, gateways::DeviceSettingsManager::MaxNameSize> deviceUrl;
    gateways::utils::fillDevicePath( deviceUrl.data(), deviceUrl.size(), toDevicePrefix( GatewayID::Cloud ), uniqueId );

    if( storage::Storage::get().isExist( deviceUrl.data() ) ) {
        auto numbersAttemps = 0;
        do {
            if( storage::Storage::get().remove( deviceUrl.data() ) ) {
                return true;
            }
        } while( numbersAttemps++ < 3 );

        return false;
    }
    return true;
}

Device DevicesStorage::restoreDevice( const std::string& id, IGateway::AddingDeviceStatus& addingStatus ) {
    std::array<char, gateways::DeviceSettingsManager::MaxNameSize> deviceUrl;
    gateways::utils::fillDevicePath( deviceUrl.data(), deviceUrl.size(), toDevicePrefix( GatewayID::Cloud ), id );

    storage::File file;
    Device dev( id.c_str() );
    if( storage::Storage::get().isExist( deviceUrl.data() ) && file.open( deviceUrl.data(), "rb" ) ) {
        if( !dev.restoreData( file ) ) {
            addingStatus = IGateway::AddingDeviceStatus::BrokenFileFormat;
            FUNC_LOGE( "restoreDevice file not found" );
        }
        else {
            addingStatus = IGateway::AddingDeviceStatus::OK;
        }
    }

    return dev;
}

}  // namespace cloud
}  // namespace gateways

#endif  // CONFIG_CLOUD_DEVICES
