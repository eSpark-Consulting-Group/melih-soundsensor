#include <GatewayDeviceRestore.h>

#include <IGateway.h>

#include <StorageFile.h>
#include <Volume.h>
#include <Storage.h>

#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <DebugUtils.h>
#include <sdkconfig.h>

namespace gateways {

void restoreDevicesInfo() {
    DIR* rootDir = ::opendir( storage::Volume::m_basePath );

    if( rootDir == nullptr ) {
        FUNC_LOGE( "Unable to open root directory for restoring device info[errno=%d]", errno );
        return;
    }
    seekdir( rootDir, 2 );  // skip current "." and upper ".." directory entries

    std::array<char, 256> url;

    size_t size = 0;
    int resultCode = 0;
    IGateway* gateway = nullptr;
    DeviceId deviceId = 0;

    storage::File deviceFile;

    struct dirent* itemInfo = nullptr;
    while( ( itemInfo = readdir( rootDir ) ) != nullptr ) {
        FUNC_LOGI( "Found file: %s", itemInfo->d_name );
        switch( itemInfo->d_type ) {
        case DT_REG: {
            size = strlen( itemInfo->d_name );
            if( size < MinimalDeviceFileNameSize ) {
                FUNC_LOGD( "'%s' name is too small for device file.", itemInfo->d_name );
                break;
            }

            if( itemInfo->d_name[size - ( DeviceFileExtensionSize + 1 )] != '.' ) {
                FUNC_LOGD( "Exstension of '%s' file has wrong format for device file.", itemInfo->d_name );
                break;
            }

            if( itemInfo->d_name[size - DeviceFileExtensionSize] != DeviceFileExtensionSymbol ) {
                FUNC_LOGD( "Exstension of '%s' file has wrong first symbol for device file.", itemInfo->d_name );
                break;
            }

            FUNC_LOGD( "Looks like device file: %s", itemInfo->d_name );

            gateway = getByDevicePrefix( itemInfo->d_name[size - 1] );
            if( gateway == nullptr ) {
                FUNC_LOGW( "Couldn't find gateway for device file %s(Gateway symbol: %c)",
                           itemInfo->d_name,
                           itemInfo->d_name[size - 1] );
                break;
            }

            auto writtenBytes =
                snprintf( url.data(), url.size(), "%s/%s", storage::Volume::m_basePath, itemInfo->d_name );
            if( writtenBytes < 0 || writtenBytes == url.size() ) {
                FUNC_LOGE( "Output has been truncated" );
            }

            if( !deviceFile.open( url.data(), "rb" ) ) {
                FUNC_LOGE(
                    "Couldn't open device file %s(Gateway: %s)", itemInfo->d_name, toString( gateway->getTypeId() ) );
                break;
            }

            itemInfo->d_name[size - ( DeviceFileExtensionSize + 1 )] = '\0';
            if( 1 != sscanf( itemInfo->d_name, "%" PRIX32, &deviceId ) ) {
                FUNC_LOGE( "Couldn't convert '%s' file name to device id", itemInfo->d_name );
                break;
            }

            FUNC_LOGD( "Try to restore device info with id: %" PRIX32 ", from '%s' file(Gateway: %s)",
                       deviceId,
                       url.data(),
                       toString( gateway->getTypeId() ) );

            switch( gateway->restoreDeviceInfo( deviceId, deviceFile ) ) {
            case IGateway::AddingDeviceStatus::BrokenFileFormat:
            case IGateway::AddingDeviceStatus::NotSupportedVersion: {
                deviceFile.close();
                resultCode = ::unlink( url.data() );
                FUNC_LOGW( "File '%s' broken and removed with status: %d", url.data(), resultCode );
            } break;

            case IGateway::AddingDeviceStatus::LeakOfResources:
                FUNC_LOGW( "Couldn't restore '%s' file. Leak of resources.", url.data() );
                break;
            case IGateway::AddingDeviceStatus::OK:
                FUNC_LOGD( "Device information was restored from file: %s", url.data() );
                break;
            };

            deviceFile.close();

        } break;

        case DT_UNKNOWN:
        case DT_DIR:
            break;
        };
    }

#if CONFIG_BOARD_TYPE_PLUG_HUB
    gateway = get( GatewayID::PlugHub );
    gateway->restoreDeviceInfo( 1, deviceFile );
#endif

#if CONFIG_BOARD_TYPE_WATER_VALVE
    gateway = get( GatewayID::WaterValve );
    gateway->restoreDeviceInfo( 1, deviceFile );
#endif

    ::closedir( rootDir );
}

void cleanDevicesInfo( gateways::GatewayID gatewaID ) {
    auto gateway = gateways::get( gatewaID );
    if( gateway != nullptr ) {
        gateway->cleanAllData();
    }
}

void printFilesDevicesInfo() {
    DIR* rootDir = ::opendir( storage::Volume::m_basePath );

    if( rootDir == nullptr ) {
        FUNC_LOGE( "Unable to open root directory for restoring device info[errno=%d]", errno );
        return;
    }

    struct dirent* itemInfo = nullptr;
    while( ( itemInfo = readdir( rootDir ) ) != nullptr ) {
        FUNC_LOGI( "Found file: %s", itemInfo->d_name );
    }

    ::closedir( rootDir );
}

}  // namespace gateways
