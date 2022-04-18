#include "sdkconfig.h"

#ifndef CONFIG_UART_API

#include "NmaJsonHandler.h"
#include "CloudNmaClient.h"

#include "CommonResources.h"
#include "FirmwareUpdater.h"
#include "HardUtils.h"
#include "NmaBroadcastsSender.h"
#include <StorageArchiveFileFactory.h>
#include "plugin_manager.h"
#include "ProvisionPeersInfoManager.h"
#include "StorageFileFormat.h"

#include "DebugUtils.h"

namespace cloud_client {

using namespace common::resources;

NmaJsonHandler::ResponceType NmaJsonHandler::onRegistered( const gason::JsonValue& json,
                                                           common::JsonStreamGenerator& gen ) {
    static_cast<CloudNmaClient&>( m_client ).onRegistered();
    return std::make_tuple( errors::NotError, StringType() );
}

NmaJsonHandler::ResponceType NmaJsonHandler::onCloudFirmwareInfoGet( const gason::JsonValue& json,
                                                                     common::JsonStreamGenerator& gen ) {
    const auto result = json( tokens::Result );
    const auto signature = result( tokens::Signature ).toString();
    const auto version = result( tokens::Version ).toString();
    const auto urls = result( tokens::Urls );
    const auto checksum_url = urls( tokens::Checksum ).toString();
    const auto firmware_url = urls( tokens::Firmware ).toString();

    if( core::FirmwareUpdater::sendUpdateNotifyIfNeeded() ) {
        FUNC_LOGI( "Firmware update completed!" );
        static_cast<CloudNmaClient&>( m_client ).onFirmwareUpdateCompleted();
        return std::make_tuple( errors::NotError, StringType() );
    }

    if( version && firmware_url && checksum_url && signature ) {
        std::string path( firmware_url );
        const auto filename = path.substr( path.find_last_of( "/\\" ) + 1 );
        if( filename.size() ) {
            if( filename.find( FIRMWARE_PREFIX ) == std::string::npos ) {
                FUNC_LOGE( "Update failed: firmware does not match the hardware settings" );
            }
            else {
                FUNC_LOGI( "Firmware update available. Version: %s, firmware: %s, checksum: %s, signature: %s",
                           version,
                           firmware_url,
                           checksum_url,
                           signature );

                if( core::FirmwareUpdater::updateNew( firmware_url, checksum_url ) ) {
                    FUNC_LOGI( "Update success" );
                }
                else {
                    FUNC_LOGE( "Update failed" );
                    static_cast<CloudNmaClient&>( m_client ).onFirmwareUpdateCompleted();
                }
            }
        }

        return std::make_tuple( errors::NotError, StringType() );
    }

    FUNC_LOGI( "No firmware update available" );
    if( core::FirmwareUpdater::isFimwareUpdateStarted() || core::FirmwareUpdater::isFirmwareUpdateInProgress() ) {
        core::FirmwareUpdater::resetFirmwareUpdateFlag();
        core::FirmwareUpdater::saveFimwareUpdateStartedFlag( false );
        nma::BroadcastsSender::sendUpdatingFirmwareError( nma::Errors::UpdateOperationsError,
                                                          hard::common::generateUniqueId() );
    }

    static_cast<CloudNmaClient&>( m_client ).onFirmwareUpdateCompleted();
    return std::make_tuple( errors::NotError, StringType() );
}

NmaJsonHandler::ResponceType NmaJsonHandler::onHubFirmwareUpdate( const gason::JsonValue& json,
                                                                  common::JsonStreamGenerator& gen ) {
    gen.writeEmptyObject() && gen.writeItemDelimiter();

    const auto params = json( tokens::Params );
    const auto version = params( tokens::Version ).toString();
    const auto source = params( tokens::Source ).toString();
    if( !version || !source ) {
        return std::make_tuple( errors::InvalidParams, StringType() );
    }

    if( core::FirmwareUpdater::update(
            static_cast<CloudNmaClient&>( m_client ).nmaServer(), std::move( version ), std::move( source ) ) ) {
        return std::make_tuple( errors::NotError, StringType() );
    }
    else {
        static_cast<CloudNmaClient&>( m_client ).onFirmwareUpdateCompleted();
    }

    return std::make_tuple( errors::ApplicationError, errors::messages::GatewayBusy );
}

#ifdef CONFIG_LOCAL_MODE
NmaJsonHandler::ResponceType NmaJsonHandler::onCloudAccessKeysControllerSync( const gason::JsonValue& json,
                                                                              common::JsonStreamGenerator& gen ) {
    auto peersProvisionManager = provision::peers::InfoManager::get();
    if( !peersProvisionManager ) {
        return std::make_tuple( errors::InternalError, StringType() );
    }
    peersProvisionManager->pars_sync_local_mode( json );
    return std::make_tuple( errors::NotError, StringType() );
}
#endif  // CONFIG_LOCAL_MODE

NmaJsonHandler::ResponceType NmaJsonHandler::onHubOfflineCredentialsUpdated( const gason::JsonValue& json,
                                                                             common::JsonStreamGenerator& gen ) {
    gen.writeEmptyObject() && gen.writeItemDelimiter();

    auto params = json( tokens::Params );
    if( !params ) {
        FUNC_LOGE( "Bad request, params does not exist" );
        return std::make_tuple( errors::InvalidRequest, StringType( errors::data::BadParams ) );
    }

    auto versionJson = params( "newVersion" );
    if( !versionJson ) {
        FUNC_LOGE( "Bad request, newVersion does not exist" );
        return std::make_tuple( errors::InvalidRequest,
                                StringType( errors::data::BadParams ) + StringType( ".newVersion" ) );
    }

    bool isSuccess{ false };
    auto newVer = versionJson.toInt( &isSuccess );
    if( !isSuccess ) {
        FUNC_LOGE( "Wrong params, newVersion field value not json int type" );
        return std::make_tuple( errors::InvalidParams,
                                StringType( errors::data::WrongParams ) + StringType( ".newVersion" ) );
    }

    auto client = provision::peers::InfoManager::get();
    if( !client ) {
        FUNC_LOGE( "Can't get provision::peers::InfoManager instance" );
        return std::make_tuple( errors::InternalError, StringType( errors::data::EzloInternaleError ) );
    }

    auto currVer = client->get_version();
    if( newVer <= currVer ) {
        FUNC_LOGE( "Wrong params, newVersion(%d) <= currentVersion(%d)", newVer, currVer );
        return std::make_tuple( errors::InvalidParams,
                                StringType( errors::data::WrongParams ) + StringType( ".newVersion" ) );
    }

    client->set_update( true );
    return std::make_tuple( errors::NotError, StringType() );
}

NmaJsonHandler::ResponceType NmaJsonHandler::onHubSoftwareUpdateExecute( const gason::JsonValue& json,
                                                                         common::JsonStreamGenerator& jsonGenerator ) {
    auto errorCode = errors::NotError;

    std::list<plugin::PluginURI> pluginURIs;
    jsonGenerator.writeEmptyObject() && jsonGenerator.writeItemDelimiter();

    const auto operationsArray = json( tokens::Params )( tokens::Actions );

    if( operationsArray && operationsArray.isArray() ) {
        for( auto& it : json( tokens::Params )( tokens::Actions ) ) {
            if( !( it.value( tokens::Operation ) && it.value( tokens::Operation ).isString() ) ||
                !( it.value( tokens::Type ) && it.value( tokens::Type ).isString() ) ||
                !( it.value( tokens::Version ) && it.value( tokens::Version ).isString() ) ||
                !( it.value( tokens::Source ) && it.value( tokens::Source ).isString() ) ||
                !( it.value( tokens::Snapshot ) && it.value( tokens::Snapshot ).isNumber() ) ) {
                return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
            }

            const char* operation = it.value( tokens::Operation ).toString();
            const char* type = it.value( tokens::Type ).toString();
            const char* version = it.value( tokens::Version ).toString();
            const char* source = it.value( tokens::Source ).toString();

            if( ( !strcmp( operation, tokens::Uprgrade ) || !strcmp( operation, tokens::Install ) ) &&
                ( strlen( source ) && strlen( type ) && strlen( operation ) ) ) {
                if( !strcmp( type, tokens::Firmware ) ) {
                    std::string url = source;
                    auto pathSuffix = url.substr( url.rfind( "/" ) + 1 );
                    url += "/" + std::string( FIRMWARE_PREFIX ) + "-" + std::string( pathSuffix ) + "-" + version;

                    std::string firmware_url = url + ".bin";
                    std::string checksum_url = url + ".sig";

                    if( !core::FirmwareUpdater::updateNew( std::move( firmware_url ), std::move( checksum_url ) ) ) {
                        FUNC_LOGE( "Firmware update failed" );
                    }
                }
                else if( !strcmp( type, tokens::Plugin ) ) {
                    plugin::PluginURI plugUri;
                    if( sscanf( version, "%hhu.%hhu", &plugUri.plugin.version.major, &plugUri.plugin.version.minor ) ==
                        2 ) {
                        if( it.value( tokens::Id ) && it.value( tokens::Id ).isString() ) {
                            plugUri.plugin.id = it.value( tokens::Id ).toString();
#ifdef TAR_FORMAT
                            plugUri.uri =
                                std::string( source ) + "/" + plugUri.plugin.id + "." + storage::TAR_ZZ_EXTENSION;
                            plugUri.format = storage::FileFormat::TAR_ZZ;
#endif
#ifdef MOLFAR_FORMAT
                            plugUri.uri =
                                std::string( source ) + "/" + plugUri.plugin.id + "." + storage::MOLFAR_ZZ_EXTENSION;
                            plugUri.format = storage::FileFormat::MOLFAR_ZZ;
#endif
                            plugUri.size = 1;

                            pluginURIs.push_back( plugUri );
                        }
                        else {
                            return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
                        }
                    }
                }
                else if( !strcmp( type, tokens::Addon ) ) {
                    return std::make_tuple( errors::MethodNotFound, StringType{} );
                }
                else {
                    return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
                }
            }
            else if( !strcmp( operation, tokens::Uninstall ) ) {
                if( !strcmp( type, tokens::Plugin ) ) {
                    bool result = false;
                    if( it.value( tokens::Id ) && it.value( tokens::Id ).isString() ) {
                        const char* id = it.value( tokens::Id ).toString();
                        auto ret = plugin::PluginManager::get().doUninstall( jsonGenerator, id, version );
                        result = ( plugin::PluginManager::Result::OK == ret );
                    }
                    if( !result ) {
                        errorCode = errors::ApplicationError;
                    }
                }
                else {
                    return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
                }
            }
        }
    }
    else {
        return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
    }

    if( !pluginURIs.empty() ) {
        auto ret = plugin::PluginManager::get().doInstall( jsonGenerator, std::move( pluginURIs ) );
        bool result = ( plugin::PluginManager::Result::OK == ret );
        if( !result ) {
            errorCode = errors::ApplicationError;
        }
    }

    return std::make_tuple( errorCode, StringType() );
}

NmaJsonHandler::ResponceType NmaJsonHandler::onHubNetworkWifiScanStart( const gason::JsonValue& json,
                                                                        common::JsonStreamGenerator& jsGen ) {
    jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();

    auto params = json( tokens::Params );
    if( !params ) {
        return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
    }

    const char* interfaceId = params( tokens::InterfaceId ).toString();
    if( !interfaceId ) {
        return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
    }

    if( 0 == std::strcmp( interfaceId, wifi::tokens::WLAN ) ) {
        return static_cast<CloudNmaClient&>( m_client ).tryStartWifiAccessPointsScan() ?
                   std::make_tuple( errors::NotError, StringType() ) :
                   std::make_tuple( errors::InternalError, errors::data::InternalOperationError );
    }

    bool result = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    result = result && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubNetworkWifiScanProgress );
    result = result && jsGen.writePropertyKey( tokens::Result );
    result = result && jsGen.writeOpeningObject();
    result = result && jsGen.writePropertyKey( tokens::Error );
    result = result && jsGen.writeOpeningObject();
    result = result && jsGen.writeProperty( tokens::Code, errors::ApplicationError );
    result = result && jsGen.writeProperty( tokens::Data, "network.wifi.scan.failed" );
    result = result && jsGen.writeProperty( tokens::Message, "Wifi scan failed" );
    result = result && jsGen.writeProperty( tokens::Reason, "command failed: No such device (-19)" );
    result = result && jsGen.writeClosingSymbol();
    result = result && jsGen.writeProperty( tokens::InstanceId, interfaceId );
    result = result && jsGen.writeProperty( tokens::Status, broadcasts::events::Failed );
    result = result && jsGen.writeClosingSymbol();
    result = result && jsGen.writeJsonEnd();
    return result ? std::make_tuple( errors::NotError, StringType() ) :
                    std::make_tuple( errors::GenJsonFailed, errors::data::InternalOperationError );
}

NmaJsonHandler::ResponceType NmaJsonHandler::onHubNetworkWifiScanStop( const gason::JsonValue& json,
                                                                       common::JsonStreamGenerator& jsGen ) {
    jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();

    return static_cast<CloudNmaClient&>( m_client ).tryStopWifiAccessPointsScan() ?
               std::make_tuple( errors::NotError, StringType() ) :
               std::make_tuple( errors::InternalError, errors::data::InternalOperationError );
}

}  // namespace cloud_client

#endif  // not CONFIG_UART_API
