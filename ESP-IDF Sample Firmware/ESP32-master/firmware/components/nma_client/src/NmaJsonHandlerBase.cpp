#include "NmaJsonHandlerBase.h"

#include "DebugUtils.h"

namespace nma {

namespace {

using namespace common::resources;

}  // namespace

NmaJsonHandlerBase::NmaJsonHandlerBase()
: m_jsListener( *this ) {
    m_data.reserve( DefaultBufSize );
    m_jsParser.setListener( &m_jsListener );
}

int NmaJsonHandlerBase::updateDefferedTask() {
    if( !hasDefferedTask() ) {
        return errors::NotError;
    }

    auto [errorCode, errorMessage] = m_taskClbk( m_context->json, m_context->jsGen );
    if( errors::Waiting == errorCode ) {
        return errors::Waiting;
    }
    else if( errors::Deffered == errorCode ) {
        return errors::Deffered;
    }

    completeResponce( m_context->requestId, m_context->json, m_context->jsGen, errorCode, std::move( errorMessage ) );
    m_taskClbk = nullptr;
    m_context = nullptr;
    m_outStream = nullptr;
    return errorCode;
}

bool NmaJsonHandlerBase::parse( common::IReadStream& readStream, common::IWriteStream::UPtr writeStream ) {
    {
        auto res = updateDefferedTask();
        if( errors::Waiting == res ) {
            return true;
        }
        else if( errors::NotError != res ) {
            return false;
        }
    }

    m_outStream = std::move( writeStream );

    char ch;
    while( true ) {
        auto readedBytes = readStream.read( &ch, 1 );
        if( 0 > readedBytes ) {
            m_outStream = nullptr;
            return false;
        }
        else if( 0 == readedBytes ) {
            m_outStream = nullptr;
            return true;
        }

        m_data.push_back( ch );
        m_jsParser.parse( ch );

        if( isNewHandler() ) {
            return true;
        }
        else if( m_isParsingFailed ) {
            m_isParsingFailed = false;
            m_outStream = nullptr;
            return false;
        }

        if( hasDefferedTask() ) {
            return true;
        }
    }
}

void NmaJsonHandlerBase::onJsonDocumentReceived() {
    if( !m_isParsingFailed ) {
        m_data.push_back( '\0' );
        parseJsonDocument();
    }

    reset();
}

void NmaJsonHandlerBase::onParseError( const int errorCode ) {
    FUNC_LOGE( "Json parse error: %d", errorCode );
    m_isParsingFailed = true;
}

void NmaJsonHandlerBase::parseJsonDocument() {
    auto context = std::make_unique<Context>( std::move( m_data ), *m_outStream );
    gason::JsonParseStatus status = gason::jsonParse( context->jsData.data(), context->json, context->jsonAlloc );
    if( gason::JSON_PARSE_OK != status ) {
        FUNC_LOGE( "Wrong json format. Error: %d", status );
        return;
    }

    {
        const char* requestIdCStr = context->json( tokens::Id ).toString();
        if( !requestIdCStr ) {
            FUNC_LOGE( "No ID field in received message" );
            return;
        }
        context->requestId = requestIdCStr;
    }

    const char* method = context->json( tokens::Method ).toString();
    if( !method ) {
        method = context->json( tokens::MsgSubclass ).toString();
    }
    if( !method ) {
        FUNC_LOGE( "No \"method\" or \"msg_subclass\" field in received message" );
        return;
    }

    // old method name dublicates list
    // modify old method name to new, deletуe old when you'll need it
    if( !strcasecmp( method, commands::ExtensionsPluginRun ) ) {
        method = commands::HubExtensionsPluginRun;
    }
    else if( !strcasecmp( method, commands::HubExtensionsPluginInfoInstalled ) ) {
        method = commands::ExtensionsPluginInfoInstalled;
    }
    else if( !strcasecmp( method, commands::HubExtensionsPluginInstallExecute ) ) {
        method = commands::ExtensionsPluginInstallExecute;
    }
    else if( !strcasecmp( method, commands::HubExtensionsPluginInstallRequest ) ) {
        method = commands::ExtensionsPluginInstallRequest;
    }
    else if( !strcasecmp( method, commands::HubExtensionsPluginUninstallExecute ) ) {
        method = commands::ExtensionsPluginUninstallExecute;
    }
    else if( !strcasecmp( method, commands::HubExtensionsPluginUninstallRequest ) ) {
        method = commands::ExtensionsPluginUninstallRequest;
    }

    processMethod( method, context );
}

void NmaJsonHandlerBase::processMethod( const char* method, std::unique_ptr<Context>& context ) {
    FUNC_LOGI( "process method: %s", method );

    auto errorCode = errors::MethodNotFound;
    StringType errorMessage;

    std::tie( errorCode, errorMessage ) = callCloudMethods( method, context->json, context->jsGen );
    if( errors::NotError == errorCode || errors::MethodNotFound != errorCode ) {
        if( !context->jsGen.writeJsonEnd() ) {
            m_isParsingFailed = true;
        }
        return;
    }

    std::tie( errorCode, errorMessage ) = callBroadcastsMethods( method, context->json, context->jsGen );
    if( errors::NotError == errorCode || errors::MethodNotFound != errorCode ) {
        return;
    }

    if( !context->jsGen.writeOpeningObject() || !context->jsGen.writeProperty( tokens::Method, method ) ||
        !context->jsGen.writeProperty( tokens::MsgId, getMsgId() ) ) {
        errorCode = errors::GenJsonFailed;
        m_isParsingFailed = true;
        return;
    }

    auto sender = context->json( tokens::Sender );
    if( sender ) {
        auto connId = sender( tokens::ConId ).toString();
        auto type = sender( tokens::Type ).toString();
        if( connId || type ) {
            context->jsGen.writePropertyKey( tokens::Sender );
            context->jsGen.writeOpeningObject();
            if( connId ) {
                context->jsGen.writeProperty( tokens::ConId, connId );
            }
            if( type ) {
                context->jsGen.writeProperty( tokens::Type, type );
            }
            context->jsGen.writeClosingSymbol();
        }
    }

    if( !context->jsGen.writePropertyKey( tokens::Result ) ) {
        FUNC_LOGE( "JsonGenerator failed! (3)" );
        m_isParsingFailed = true;
        return;
    }

    std::tie( errorCode, errorMessage ) = callHubMethods( method, context->json, context->jsGen );
    if( errors::Deffered == errorCode ) {
        m_context = std::move( context );
        return;
    }

    completeResponce( context->requestId, context->json, context->jsGen, errorCode, std::move( errorMessage ) );
}

void NmaJsonHandlerBase::completeResponce( const StringType& requestId,
                                           const gason::JsonValue& json,
                                           common::JsonStreamGenerator& jsGen,
                                           int errorCode,
                                           StringType&& errorMessage ) {
    if( errors::NotError == errorCode ) {
        jsGen.writeProperty( tokens::Error, nullptr );
    }
    else {
        if( errors::GenJsonFailed == errorCode ) {
            return;
        }

        if( errors::MethodNotFound == errorCode ) {
            jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();
        }

        if( jsGen.writePropertyKey( tokens::Error ) && jsGen.writeOpeningObject() &&
            jsGen.writeProperty( tokens::Code, errorCode ) ) {
            if( errorMessage.empty() ) {
                if( errors::MethodNotFound == errorCode ) {
                    jsGen.writeProperty( tokens::Message, errors::messages::UnknownMethod );
                }
            }
            else {
                jsGen.writeProperty( tokens::Message, errorMessage );
            }

            jsGen.writeClosingSymbol();
        }
    }

    jsGen.writeProperty( tokens::Id, requestId ) && jsGen.writeJsonEnd();
}

void NmaJsonHandlerBase::reset() {
    m_data.resize( DefaultBufSize );
    m_data.clear();
    m_jsParser.reset();
}

NmaJsonHandlerBase::ResponceType NmaJsonHandlerBase::callCloudMethods( const char* method,
                                                                       const gason::JsonValue& json,
                                                                       common::JsonStreamGenerator& jsGen ) {
    auto errorCode = errors::MethodNotFound;
    StringType errorMessage;

    switch( *method ) {
    case 'c':
    case 'C':
        if( !strcasecmp( method, commands::CloudFirmwareInfoGet ) ) {
            std::tie( errorCode, errorMessage ) = onCloudFirmwareInfoGet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::CloudStorageControllerToken ) ) {
            std::tie( errorCode, errorMessage ) = onCloudStorageControllerToken( json, jsGen );
        }

#ifdef CONFIG_LOCAL_MODE
        else if( !strcasecmp( method, commands::CloudAccessKeysControllerSync ) ) {
            std::tie( errorCode, errorMessage ) = onCloudAccessKeysControllerSync( json, jsGen );
        }
#endif  // CONFIG_LOCAL_MODE

#ifdef CONFIG_CLOUD_DEVICES
        if( !strcasecmp( method, "cloud.abstracts.updated" ) ) {
            std::tie( errorCode, errorMessage ) = onCloudAbstractsUpdated( json, jsGen );
        }

        else if( !strcasecmp( method, "cloud.abstracts.added" ) ) {
            std::tie( errorCode, errorMessage ) = onCloudAbstractsAdded( json, jsGen );
        }

        else if( !strcasecmp( method, "cloud.abstracts.deleted" ) ) {
            std::tie( errorCode, errorMessage ) = onCloudAbstractsDeleted( json, jsGen );
        }

        else if( !strcasecmp( method, commands::CloudControllerAbstractsList ) ) {
            std::tie( errorCode, errorMessage ) = onCloudControllerAbstractSetResponse( json, jsGen );
        }
#endif  // CONFIG_CLOUD_DEVICES

        break;
    case 'n':
    case 'N':
        if( !strcasecmp( method, commands::CloudTimeZonesGet ) ) {
            std::tie( errorCode, errorMessage ) = onCloudTimeZonesGet( json, jsGen );
        }
        break;
    case 'r':
    case 'R':
        if( !strcasecmp( method, commands::Registered ) ) {
            std::tie( errorCode, errorMessage ) = onRegistered( json, jsGen );
        }
        else if( !strcasecmp( method, commands::Register ) ) {
            std::tie( errorCode, errorMessage ) = onRegisterNmaClient( json, jsGen );
        }
        break;
    }

    return std::make_tuple( errorCode, std::move( errorMessage ) );
}

NmaJsonHandlerBase::ResponceType NmaJsonHandlerBase::callBroadcastsMethods( const char* method,
                                                                            const gason::JsonValue& json,
                                                                            common::JsonStreamGenerator& jsGen ) {
    auto errorCode = errors::MethodNotFound;
    StringType errorMessage;

    switch( *method ) {
    case 'h':
    case 'H':
        if( !strcasecmp( method, broadcasts::SceneRunProgress ) ) {
            std::tie( errorCode, errorMessage ) = onHubSceneRunProgressBroadcast( json, jsGen );
        }
        break;
    }

    return std::make_tuple( errorCode, std::move( errorMessage ) );
}

NmaJsonHandlerBase::ResponceType NmaJsonHandlerBase::callHubMethods( const char* method,
                                                                     const gason::JsonValue& json,
                                                                     common::JsonStreamGenerator& jsGen ) {
    auto errorCode = errors::MethodNotFound;
    StringType errorMessage;

    switch( method[0] ) {
    case 'e':
    case 'E':
        if( !strcasecmp( method, commands::ExtensionsPluginInfoInstalled ) ) {
            std::tie( errorCode, errorMessage ) = onExtensionsPluginInfoInstalled( json, jsGen );
        }

        else if( !strcasecmp( method, commands::ExtensionsPluginInstallExecute ) ) {
            std::tie( errorCode, errorMessage ) = onExtensionsPluginInstallExecute( json, jsGen );
        }

        else if( !strcasecmp( method, commands::ExtensionsPluginInstallRequest ) ) {
            std::tie( errorCode, errorMessage ) = onExtensionsPluginInstallRequest( json, jsGen );
        }

        else if( !strcasecmp( method, commands::ExtensionsPluginUninstallExecute ) ) {
            std::tie( errorCode, errorMessage ) = onExtensionsPluginUninstallExecute( json, jsGen );
        }

        else if( !strcasecmp( method, commands::ExtensionsPluginUninstallRequest ) ) {
            std::tie( errorCode, errorMessage ) = onExtensionsPluginUninstallRequest( json, jsGen );
        }
        break;

    case 'h':
    case 'H':
        if( !strcasecmp( method, commands::HubAddonsZwaveNodesProvisioningAdd ) ) {
            std::tie( errorCode, errorMessage ) = onHubAddonsZwaveNodesProvisioningAdd( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubAddonsZwaveNodesProvisioningRemove ) ) {
            std::tie( errorCode, errorMessage ) = onHubAddonsZwaveNodesProvisioningRemove( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubAddonsZwaveNodesProvisioningSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubAddonsZwaveNodesProvisioningSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubAddonsZwaveNodesProvisioningList ) ) {
            std::tie( errorCode, errorMessage ) = onHubAddonsZwaveNodesProvisioningList( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubOfflineCredentialsUpdated ) ) {
            std::tie( errorCode, errorMessage ) = onHubOfflineCredentialsUpdated( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubExtensionsPluginRun ) ) {
            std::tie( errorCode, errorMessage ) = onHubExtensionsPluginRun( json, jsGen );
        }

        else if( !strcasecmp( method, common::resources::commands::HubCoordinatesGet ) ) {
            std::tie( errorCode, errorMessage ) = onHubCoordinatesGet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubSoftwareUpdateExecute ) ) {
            std::tie( errorCode, errorMessage ) = onHubSoftwareUpdateExecute( json, jsGen );
        }

        else if( !strcasecmp( method, common::resources::commands::HubInfoGet ) ) {
            std::tie( errorCode, errorMessage ) = onHubInfoGet( json, jsGen );
        }

        else if( !strcasecmp( method, common::resources::commands::HubCoordinatesSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubCoordinatesSet( json, jsGen );
        }

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
        else if( !strcasecmp( method, commands::HubDetectionDeviceAdd ) ) {
            std::tie( errorCode, errorMessage ) = onHubDetectionDeviceAdd( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubDetectionDeviceRemove ) ) {
            std::tie( errorCode, errorMessage ) = onHubDetectionDeviceRemove( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubDetectionDevicesList ) ) {
            std::tie( errorCode, errorMessage ) = onHubDetectionDevicesList( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubDetectionDeviceSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubDetectionDeviceSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubDetectionDevicesScanStart ) ) {
            std::tie( errorCode, errorMessage ) = onHubDetectionDevicesScanStart( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubDetectionDevicesScanStop ) ) {
            std::tie( errorCode, errorMessage ) = onHubDetectionDevicesScanStop( json, jsGen );
        }
#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

        else if( !strcasecmp( method, common::resources::commands::HubFavoriteList ) ) {
            std::tie( errorCode, errorMessage ) = onHubFavoriteList( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubFavoriteSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubFavoriteSet( json, jsGen );
        }

        else if( !strcasecmp( method, common::resources::commands::HubFirmwareUpdate ) ) {
            std::tie( errorCode, errorMessage ) = onHubFirmwareUpdate( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubFeaturesList ) ) {
            std::tie( errorCode, errorMessage ) = onHubFeaturesList( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubFeatureStatusSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubFeatureStatusSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubGatewaysList ) ) {
            std::tie( errorCode, errorMessage ) = onHubGatewaysList( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubDeviceForceRemove ) ) {
            std::tie( errorCode, errorMessage ) = onHubDeviceForceRemove( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubDeviceArmedSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubDeviceArmedSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubDevicesList ) ) {
            std::tie( errorCode, errorMessage ) = onHubDevicesList( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubDeviceNameSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubDeviceNameSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubDeviceRoomSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubDeviceRoomSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubItemsList ) ) {
            std::tie( errorCode, errorMessage ) = onHubItemsList( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubDeviceSettingsList ) ) {
            std::tie( errorCode, errorMessage ) = onHubDeviceSettingsList( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubGeteweySettingsList ) ) {
            std::tie( errorCode, errorMessage ) = onHubGatewaySettingsList( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubItemDictionaryClear ) ) {
            std::tie( errorCode, errorMessage ) = onHubItemDictionaryClear( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubItemDictionaryValueAdd ) ) {
            std::tie( errorCode, errorMessage ) = onHubItemDictionaryValueAdd( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubItemDictionaryValueSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubItemDictionaryValueSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubDataValueListRequest ) ) {
            std::tie( errorCode, errorMessage ) = onHubDataListRequest( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubSettingsListRequest ) ) {
            std::tie( errorCode, errorMessage ) = onHubSettingsListRequest( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubSettingValueSetRequest ) ) {
            std::tie( errorCode, errorMessage ) = onHubSettingValueSetRequest( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubSettingValueResetRequest ) ) {
            std::tie( errorCode, errorMessage ) = onHubSettingValueResetRequest( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubItemDictionaryValueRemove ) ) {
            std::tie( errorCode, errorMessage ) = onHubItemDictionaryValueRemove( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubItemValueSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubItemValueSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubDeviceSettingValueSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubDeviceSettingValueSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubGatewaySettingValueSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubGatewaySettingValueSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubDeviceSettingReset ) ) {
            std::tie( errorCode, errorMessage ) = onHubDeviceSettingReset( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubDeviceSettingDictionaryValueSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubDeviceSettingDictionaryValueSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubItemValuePercentApply ) ) {
            std::tie( errorCode, errorMessage ) = onHubItemValuePercentApply( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubModesGet ) ) {
            std::tie( errorCode, errorMessage ) = onHubModesGet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubModesCurrentGet ) ) {
            std::tie( errorCode, errorMessage ) = onHubModesCurrentGet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubModesAlarmDelaySet ) ) {
            std::tie( errorCode, errorMessage ) = onHubModesAlarmDelaySet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubModesSwitch ) ) {
            std::tie( errorCode, errorMessage ) = onHubModesSwitch( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubModesSwitchCancel ) ) {
            std::tie( errorCode, errorMessage ) = onHubModesSwitchCancel( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubModesSwitchToDelaySet ) ) {
            std::tie( errorCode, errorMessage ) = onHubModesSwitchToDelaySet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubModesNotificationNotifyAllSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubModesNotificationNotifyAllSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubModesNotificationsSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubModesNotificationsSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubModesNotificationAdd ) ) {
            std::tie( errorCode, errorMessage ) = onHubModesNotificationAdd( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubModesNotificationRemove ) ) {
            std::tie( errorCode, errorMessage ) = onHubModesNotificationRemove( json, jsGen );
        }

#ifdef CONFIG_SPIRAM
        else if( !strcasecmp( method, commands::HubBridgeControllerConnectionSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubBridgeControllerConnectionSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubBridgeControllerConnectionList ) ) {
            std::tie( errorCode, errorMessage ) = onHubBridgeControllerConnectionList( json, jsGen );
        }
#endif  // CONFIG_SPIRAM

        else if( !strcasecmp( method, commands::HubModesDisarmedDefaultSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubModesDisarmedDefaultSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubModesDisarmedDevicesAdd ) ) {
            std::tie( errorCode, errorMessage ) = onHubModesDisarmedDevicesAdd( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubModesDisarmedDevicesRemove ) ) {
            std::tie( errorCode, errorMessage ) = onHubModesDisarmedDevicesRemove( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubProvisionInfoSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubProvisionInfoSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubModesAlarmsOffAdd ) ) {
            std::tie( errorCode, errorMessage ) = onHubModesAlarmsOffAdd( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubModesAlarmsOffRemove ) ) {
            std::tie( errorCode, errorMessage ) = onHubModesAlarmsOffRemove( json, jsGen );
        }

#ifndef CONFIG_UART_API
        else if( !strcasecmp( method, "hub.network.wifi.try_connect" ) ) {
            std::tie( errorCode, errorMessage ) = onHubNetworkWifiTryConnect( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubNetworkEnabledSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubNetworkEnabledSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubNetworkGet ) ) {
            std::tie( errorCode, errorMessage ) = onHubNetworkGet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubNetworkReset ) ) {
            std::tie( errorCode, errorMessage ) = onHubNetworkReset( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubNetworkWifiScanStart ) ) {
            std::tie( errorCode, errorMessage ) = onHubNetworkWifiScanStart( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubNetworkWifiScanStop ) ) {
            std::tie( errorCode, errorMessage ) = onHubNetworkWifiScanStop( json, jsGen );
        }
#endif  // not CONFIG_UART_API

        else if( !strcasecmp( method, commands::HubReset ) ) {
            std::tie( errorCode, errorMessage ) = onHubReset( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubLogRemoteSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubLogRemoteSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubLogLocalSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubLogLocalSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubRoomAllDelete ) ) {
            std::tie( errorCode, errorMessage ) = onHubRoomAllDelete( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubRoomCreate ) ) {
            std::tie( errorCode, errorMessage ) = onHubRoomCreate( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubRoomDelete ) ) {
            std::tie( errorCode, errorMessage ) = onHubRoomDelete( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubRoomGet ) ) {
            std::tie( errorCode, errorMessage ) = onHubRoomGet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubRoomList ) ) {
            std::tie( errorCode, errorMessage ) = onHubRoomList( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubRoomNameSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubRoomNameSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubRoomOrderSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubRoomOrderSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubRoomViewSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubRoomViewSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubScenesCreate ) ) {
            std::tie( errorCode, errorMessage ) = onHubScenesCreate( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubScenesGlobalCreate ) ) {
            std::tie( errorCode, errorMessage ) = onHubGlobalScenesCreate( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubScenesDelete ) ) {
            std::tie( errorCode, errorMessage ) = onHubSceneDelete( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubScenesGlobalDelete ) ) {
            std::tie( errorCode, errorMessage ) = onHubSceneGlobalDelete( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubScenesEdit ) ) {
            std::tie( errorCode, errorMessage ) = onHubScenesEdit( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubScenesGlobalEdit ) ) {
            std::tie( errorCode, errorMessage ) = onHubScenesGlobalEdit( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubScenesEnabledSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubScenesEnabledSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubScenesGlobalEnabledSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubScenesGlobalEnabledSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubScenesGet ) ) {
            std::tie( errorCode, errorMessage ) = onHubScenesGet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubScenesList ) ) {
            std::tie( errorCode, errorMessage ) = onHubScenesList( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubScenesGlobalList ) ) {
            std::tie( errorCode, errorMessage ) = onHubScenesGlobalList( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubScenesNotificationAdd ) ) {
            std::tie( errorCode, errorMessage ) = onHubScenesNotificationAdd( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubScenesNotificationRemove ) ) {
            std::tie( errorCode, errorMessage ) = onHubScenesNotificationRemove( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubScenesRoomSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubScenesRoomSet( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubScenesRun ) ) {
            std::tie( errorCode, errorMessage ) = onHubScenesRun( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubScenesGlobalRun ) ) {
            std::tie( errorCode, errorMessage ) = onHubScenesGlobalRun( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubScenesTimeList ) ) {
            std::tie( errorCode, errorMessage ) = onHubScenesTimeList( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubScenesTriggerDevicesList ) ) {
            std::tie( errorCode, errorMessage ) = onHubScenesTriggerDevicesList( jsGen );
        }

        else if( !strcasecmp( method, commands::HubServerRequest ) ) {
            std::tie( errorCode, errorMessage ) = onHubServerRequest( json, jsGen );
        }

        else if( !strcasecmp( method, common::resources::commands::HubTimeLocationGet ) ) {
            std::tie( errorCode, errorMessage ) = onHubTimeLocationGet( json, jsGen );
        }

        else if( !strcasecmp( method, common::resources::commands::HubTimeLocationSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubTimeLocationSet( json, jsGen );
        }

        else if( !strcasecmp( method, common::resources::commands::HubTimeLocationList ) ) {
            std::tie( errorCode, errorMessage ) = onHubTimeLocationList( json, jsGen );
        }

        else if( !strcasecmp( method, common::resources::commands::HubDevicesServiceNotificationsSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubDevicesServiceNotificationsSet( json, jsGen );
        }

        else if( !strcasecmp( method, common::resources::commands::HubSoftwareInfoGet ) ) {
            std::tie( errorCode, errorMessage ) = onHubSoftwareInfoGet( json, jsGen );
        }

        else if( !strcasecmp( method, common::resources::commands::HubScenesBlocksList ) ) {
            std::tie( errorCode, errorMessage ) = onHubScenesBlocksList( json, jsGen );
        }

        else if( !strcasecmp( method, common::resources::commands::HubScenesNameSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubScenesNameSet( json, jsGen );
        }

        else if( !strcasecmp( method, common::resources::commands::HubScenesHouseModesSet ) ) {
            std::tie( errorCode, errorMessage ) = onHubScenesHouseModesSet( json, jsGen );
        }

        else if( !strcasecmp( method, common::resources::commands::HubReboot ) ) {
            std::tie( errorCode, errorMessage ) = onHubReboot( json, jsGen );
        }

        else if( !strcasecmp( method, common::resources::commands::HubScenesActionBlockTest ) ) {
            std::tie( errorCode, errorMessage ) = onHubScenesActionBlockTest( json, jsGen );
        }

        else if( !strcasecmp( method, common::resources::commands::HubDeviceFirmwareUpdate ) ) {
            std::tie( errorCode, errorMessage ) = onHubDeviceFirmwareUpdate( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubOfflineLoginUi ) ) {
            std::tie( errorCode, errorMessage ) = onHubOfflineLoginUi( json, jsGen );
        }

        else if( !strcasecmp( method, commands::HubOfflineLoginHub ) ) {
            std::tie( errorCode, errorMessage ) = onHubOfflineLoginHub( json, jsGen );
        }

        [[fallthrough]]

            default : break;
    }

    return std::make_tuple( errorCode, std::move( errorMessage ) );
}

}  // namespace nma
