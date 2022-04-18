
#include <DeviceSettingsManager.h>

#include <JsonStreamGenerator.h>
#include <CommonResources.h>

#include <StorageFile.h>
#include <Storage.h>
#include <Volume.h>

#include <NmaBroadcastsSender.h>

#include <DebugUtils.h>
#include <Utils.h>

#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

namespace gateways {

constexpr char DeviceSettingsManager::DeviceSettingsFileTemplate[];
constexpr uint8_t DeviceSettingsManager::StorageMajorVersion;
constexpr uint8_t DeviceSettingsManager::StorageMinorVersion;

DeviceSettingsManager& DeviceSettingsManager::get() {
    static DeviceSettingsManager settingsManager;
    return settingsManager;
}

bool DeviceSettingsManager::fillDeviceSettingPath( char* buffer,
                                                   size_t maxBufferSize,
                                                   const DeviceId& deviceId,
                                                   GatewayID gatewayId ) const {
    return snprintf( buffer,
                     maxBufferSize,
                     DeviceSettingsFileTemplate,
                     storage::Volume::m_basePath,
                     deviceId,
                     DeviceSettingsFileExtensionSymbol,
                     toDevicePrefix( gatewayId ) ) > 0;
}

DeviceSettingsManager::Result DeviceSettingsManager::openStorageForAdding( storage::File& file,
                                                                           const DeviceId& deviceId,
                                                                           GatewayID gatewayId ) {
    std::array<char, 32> filePath;

    auto result = fillDeviceSettingPath( filePath.data(), filePath.size(), deviceId, gatewayId );
    if( result && file.open( filePath.data(), "rb+" ) ) {
        result = readAndCheckVersion( file );
        if( result ) {
            updateVersionHeader( file );
            return result ? Result::OK : Result::SystemError;
        }

        file.close();
        storage::Storage::get().remove( filePath.data() );
        result = true;
    }

    return ( result && file.open( filePath.data(), "wb+" ) && writeVersionHeader( file ) ) ? Result::OK :
                                                                                             Result::SystemError;
}

DeviceSettingsManager::Result DeviceSettingsManager::openStorageForReading( storage::File& file,
                                                                            const DeviceId& deviceId,
                                                                            GatewayID gatewayId ) {
    std::array<char, 32> filePath;

    auto result = fillDeviceSettingPath( filePath.data(), filePath.size(), deviceId, gatewayId );
    if( result && file.open( filePath.data(), "rb" ) ) {
        if( readAndCheckVersion( file ) ) {
            return Result::OK;
        }
        else {
            file.close();
            storage::Storage::get().remove( filePath.data() );
        }
    }

    return Result::FileNotExist;
}

bool DeviceSettingsManager::readAndCheckVersion( storage::File& file ) {
    uint8_t version = 0;
    auto result = file.read( version );
    result = result && ( version == StorageMajorVersion );

    result = result && file.read( version );
    result = result && ( version <= StorageMinorVersion );

    return result;
}

bool DeviceSettingsManager::writeVersionHeader( storage::File& file ) {
    return file.write( StorageMajorVersion ) && file.write( StorageMinorVersion );
}

bool DeviceSettingsManager::updateVersionHeader( storage::File& file ) {
    return fseek( file.get(), 0, SEEK_SET ) == 0 && writeVersionHeader( file );
}

DeviceSettingsManager::Result DeviceSettingsManager::setDeviceName( const DeviceId& deviceId,
                                                                    GatewayID gatewayId,
                                                                    const char* name ) {
    storage::File file;
    bool& result = file.linkToTransactionStatus();
    auto resultCode = openStorageForAdding( file, deviceId, gatewayId );
    if( resultCode != Result::OK ) {
        return resultCode;
    }

    result = file.writeStringAdjustedBySize( name, MaxNameSize );
    // SPIFFS cannot detect end of file correctly. Only seek or read operation detect end of file.
    if( result && ( file.eof() || !file.skip( sizeof( core::PagesManager::PageId ) ) ) ) {
        static const core::PagesManager::PageId emptyPageId = 0;
        result = file.write( emptyPageId );
    }

    file.close();

    return result ? Result::OK : Result::SystemError;
}

DeviceSettingsManager::Result DeviceSettingsManager::moveDeviceToPage( const DeviceId& deviceId,
                                                                       GatewayID gatewayId,
                                                                       const core::PagesManager::PageId& pageId ) {
    storage::File file;
    bool& result = file.linkToTransactionStatus();
    auto resultCode = openStorageForAdding( file, deviceId, gatewayId );
    if( resultCode != Result::OK ) {
        return resultCode;
    }

    if( file.eof() ) {
        result = file.writeStringAdjustedBySize( "", 0, MaxNameSize );
    }
    else {
        result = file.skipAdjustedString( MaxNameSize );
        if( !result ) {
            result = file.writeStringAdjustedBySize( "", 0, MaxNameSize );
        }
    }

    result = result && file.write( pageId );
    file.close();

    return result ? Result::OK : Result::SystemError;
}

DeviceSettingsManager::Result DeviceSettingsManager::readDeviceSettings( const DeviceId& deviceId,
                                                                         GatewayID gatewayId,
                                                                         char* name,
                                                                         size_t maxNameSize,
                                                                         core::PagesManager::PageId& pageId ) {
    storage::File file;
    auto resultCode = openStorageForReading( file, deviceId, gatewayId );
    if( resultCode != Result::OK ) {
        return resultCode;
    }

    assert( maxNameSize >= MaxNameSize );

    auto result = file.readStringAdjustedBySize( name, MaxNameSize );
    result = result && file.read( pageId );

    if( result ) {
        return Result::OK;
    }

    name[0] = '\0';
    pageId = 0;

    return Result::SystemError;
}

DeviceSettingsManager::Result DeviceSettingsManager::readDeviceName( const DeviceId& deviceId,
                                                                     GatewayID gatewayId,
                                                                     std::string& nameBuffer ) {
    storage::File file;
    auto resultCode = openStorageForReading( file, deviceId, gatewayId );
    if( resultCode != Result::OK ) {
        return resultCode;
    }

    auto result = file.readString( nameBuffer );
    return result ? Result::OK : Result::SystemError;
}

DeviceSettingsManager::Result DeviceSettingsManager::readPageId( const DeviceId& deviceId,
                                                                 GatewayID gatewayId,
                                                                 core::PagesManager::PageId& pageId ) {
    storage::File file;
    auto resultCode = openStorageForReading( file, deviceId, gatewayId );
    if( resultCode != Result::OK ) {
        return resultCode;
    }

    auto result = file.skipAdjustedString( MaxNameSize );
    result = result && file.read( pageId );

    if( result ) {
        return Result::OK;
    }

    pageId = 0;

    return Result::SystemError;
}

DeviceSettingsManager::Result DeviceSettingsManager::onDeviceRemoved( const DeviceId& deviceId, GatewayID gatewayId ) {
    std::array<char, 32> filePath;

    auto result = fillDeviceSettingPath( filePath.data(), filePath.size(), deviceId, gatewayId );
    if( result ) {
        storage::Storage::get().remove( filePath.data() );
        return Result::OK;
    }

    return Result::SystemError;
}

/*
 * This function returns a list with devices IDs assigned to a target Page.
 */
std::list<DeviceId> DeviceSettingsManager::getDevicesByPage( const core::PagesManager::PageId& pageId ) {
    DIR* rootDir = ::opendir( storage::Volume::m_basePath );

    std::list<DeviceId> result;

    if( rootDir == nullptr ) {
        FUNC_LOGE( "Unable to open root directory for reading device info[errno=%d]", errno );
        return result;
    }

    size_t size = 0;
    DeviceId deviceId = 0;
    GatewayID gatewayId = GatewayID::Undefined;
    core::PagesManager::PageId currentPageId = 0;

    storage::File settingsFile;

    struct dirent* itemInfo = nullptr;
    while( ( itemInfo = readdir( rootDir ) ) != nullptr ) {
        switch( itemInfo->d_type ) {
        case DT_REG: {
            size = strlen( itemInfo->d_name );
            if( size < MinimalDeviceSettingsFileNameSize ) {
                break;
            }

            if( itemInfo->d_name[size - ( DeviceSettingsFileExtensionSize + 1 )] != '.' ) {
                break;
            }

            if( itemInfo->d_name[size - DeviceSettingsFileExtensionSize] != DeviceSettingsFileExtensionSymbol ) {
                break;
            }

            gatewayId = toGatewayIDByDevicePrefix( itemInfo->d_name[size - 1] );
            if( gatewayId == GatewayID::Undefined ) {
                break;
            }

            itemInfo->d_name[size - ( DeviceSettingsFileExtensionSize + 1 )] = '\0';
            if( 1 != sscanf( itemInfo->d_name, "%" PRIX32, &deviceId ) ) {
                break;
            }

            if( openStorageForReading( settingsFile, deviceId, gatewayId ) != Result::OK ) {
                break;
            }

            if( !settingsFile.skipAdjustedString( MaxNameSize ) ) {
                break;
            }

            if( settingsFile.read( currentPageId ) ) {
                if( currentPageId == pageId ) {
                    result.push_back( deviceId );
                }
            }

            settingsFile.close();

        } break;

        case DT_UNKNOWN:
        case DT_DIR: {
            FUNC_LOGW( "'%s' has not supported type.", itemInfo->d_name );
        } break;
        };
    }

    ::closedir( rootDir );

    return result;
}

DeviceSettingsManager::Result DeviceSettingsManager::onItemRemoved( const uint32_t& itemId, bool pageObject ) {
    DIR* rootDir = ::opendir( storage::Volume::m_basePath );

    if( rootDir == nullptr ) {
        FUNC_LOGE( "Unable to open root directory for restoring device info[errno=%d]", errno );
        return Result::SystemError;
    }

    size_t size = 0;
    DeviceId deviceId = 0;
    GatewayID gatewayId = GatewayID::Undefined;
    core::PagesManager::PageId restoredPageId = 0;

    storage::File settingsFile;

    struct dirent* itemInfo = nullptr;
    while( ( itemInfo = readdir( rootDir ) ) != nullptr ) {
        FUNC_LOGI( "Found file: %s", itemInfo->d_name );
        switch( itemInfo->d_type ) {
        case DT_REG: {
            size = strlen( itemInfo->d_name );
            if( size < MinimalDeviceSettingsFileNameSize ) {
                break;
            }

            if( itemInfo->d_name[size - ( DeviceSettingsFileExtensionSize + 1 )] != '.' ) {
                break;
            }

            if( itemInfo->d_name[size - DeviceSettingsFileExtensionSize] != DeviceSettingsFileExtensionSymbol ) {
                break;
            }

            FUNC_LOGI( "Looks like device settings file: %s", itemInfo->d_name );

            gatewayId = toGatewayIDByDevicePrefix( itemInfo->d_name[size - 1] );
            if( gatewayId == GatewayID::Undefined ) {
                break;
            }

            itemInfo->d_name[size - ( DeviceSettingsFileExtensionSize + 1 )] = '\0';
            if( 1 != sscanf( itemInfo->d_name, "%" PRIX32, &deviceId ) ) {
                break;
            }

            if( openStorageForReading( settingsFile, deviceId, gatewayId ) != Result::OK ) {
                break;
            }

            if( !settingsFile.skipAdjustedString( MaxNameSize ) ) {
                break;
            }

            if( settingsFile.read( restoredPageId ) && pageObject ) {
                settingsFile.close();
                if( ( itemId == 0 ) || ( itemId == restoredPageId ) ) {
                    restoredPageId = 0;
                    if( moveDeviceToPage( deviceId, gatewayId, restoredPageId ) == Result::OK ) {
                        nma::BroadcastsSender::sendFoDevicePageUpdated( gatewayId, deviceId, restoredPageId );
                    }
                    else {
                        FUNC_LOGE(
                            "Cannot reset '%" PRIX32 "' page, in '%s' settings file.", itemId, itemInfo->d_name );
                        ::closedir( rootDir );
                        return Result::SystemError;
                    }
                }
            }

            settingsFile.close();

        } break;

        case DT_UNKNOWN:
        case DT_DIR: {
            FUNC_LOGW( "'%s' has not supported type.", itemInfo->d_name );
        } break;
        };
    }

    ::closedir( rootDir );

    return Result::OK;
}

DeviceSettingsManager::Result DeviceSettingsManager::onAllPagesRemoved() {
    return onItemRemoved( 0 );
}

}  // namespace gateways
