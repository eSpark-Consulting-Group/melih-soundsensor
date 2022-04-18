#pragma once

#include "sdkconfig.h"

#include "IReadStream.h"
#include "IWriteStream.h"
#include "CommonResources.h"

#include <stddef.h>
#include <stdint.h>
#include <memory>
#include <string>
#include <tuple>

namespace bus {
class IBusClient;
}  // namespace bus

namespace common {
class JsonStreamGenerator;
}  // namespace common

namespace gason {
class JsonValue;
class JsonAllocator;
}  // namespace gason

namespace nma {

class INmaJsonHandler {
public:
    using UPtr = std::unique_ptr<INmaJsonHandler>;
    using StringType = std::string;
    using ResponceType = std::tuple<int, StringType>;

    INmaJsonHandler();

    virtual ~INmaJsonHandler() = default;

    auto getMsgId() {
        return ++m_msgId;
    }

    virtual bool parse( common::IReadStream& readStream, common::IWriteStream::UPtr writeStream ) = 0;

    virtual void parseJsonDocument() = 0;

    virtual void reset() = 0;

    virtual bool hasDefferedTask() const = 0;

    virtual int updateDefferedTask() = 0;

#ifdef CONFIG_CLOUD_DEVICES
    virtual ResponceType onCloudAbstractsUpdated( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onCloudAbstractsAdded( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onCloudAbstractsDeleted( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onCloudControllerAbstractSetResponse( const gason::JsonValue& json,
                                                               common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }
#endif  // CONFIG_CLOUD_DEVICES

#ifdef CONFIG_LOCAL_MODE
    virtual ResponceType onCloudAccessKeysControllerSync( const gason::JsonValue& json,
                                                          common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }
#endif  // CONFIG_LOCAL_MODE

    virtual ResponceType onCloudFirmwareInfoGet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onCloudTimeZonesGet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onCloudStorageControllerToken( const gason::JsonValue& json,
                                                        common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onExtensionsPluginInfoInstalled( const gason::JsonValue& json,
                                                          common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onExtensionsPluginInstallExecute( const gason::JsonValue& json,
                                                           common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onExtensionsPluginInstallRequest( const gason::JsonValue& json,
                                                           common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onExtensionsPluginUninstallExecute( const gason::JsonValue& json,
                                                             common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onExtensionsPluginUninstallRequest( const gason::JsonValue& json,
                                                             common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubAddonsZwaveNodesProvisioningAdd( const gason::JsonValue& json,
                                                               common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubAddonsZwaveNodesProvisioningRemove( const gason::JsonValue& json,
                                                                  common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubAddonsZwaveNodesProvisioningSet( const gason::JsonValue& json,
                                                               common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubAddonsZwaveNodesProvisioningList( const gason::JsonValue& json,
                                                                common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubOfflineCredentialsUpdated( const gason::JsonValue& json,
                                                         common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubExtensionsPluginRun( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubCoordinatesGet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubSoftwareUpdateExecute( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubInfoGet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubCoordinatesSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubFavoriteList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubFavoriteSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubFirmwareUpdate( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubFeaturesList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubFeatureStatusSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubGatewaysList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubDeviceForceRemove( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubDeviceArmedSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubServerRequest( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubScenesCreate( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubGlobalScenesCreate( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubScenesEdit( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubScenesGlobalEdit( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubSceneDelete( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubSceneGlobalDelete( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubScenesNameSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubScenesHouseModesSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubScenesEnabledSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubScenesGlobalEnabledSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubScenesList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubScenesGlobalList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubScenesBlocksList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubScenesRun( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubSettingsListRequest( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubSettingValueSetRequest( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubSettingValueResetRequest( const gason::JsonValue& json,
                                                        common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubSceneRunProgressBroadcast( const gason::JsonValue& json,
                                                         common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubScenesGlobalRun( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubScenesGet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubScenesTimeList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubScenesTriggerDevicesList( common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubDevicesList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
    virtual ResponceType onHubDetectionDeviceAdd( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubDetectionDevicesList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubDetectionDeviceRemove( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubDetectionDeviceSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubDetectionDevicesScanStart( const gason::JsonValue& json,
                                                         common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubDetectionDevicesScanStop( const gason::JsonValue& json,
                                                        common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }
#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

    virtual ResponceType onHubDeviceSettingDictionaryValueSet( const gason::JsonValue& json,
                                                               common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubDeviceSettingReset( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubDeviceNameSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubDeviceRoomSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubItemsList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubDeviceSettingsList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubGatewaySettingsList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubItemDictionaryClear( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubItemDictionaryValueAdd( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubItemDictionaryValueSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubDataListRequest( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubItemDictionaryValueRemove( const gason::JsonValue& json,
                                                         common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubItemValueSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubDeviceSettingValueSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubGatewaySettingValueSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubItemValuePercentApply( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubModesGet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubModesCurrentGet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubModesAlarmDelaySet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubModesSwitch( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubModesSwitchCancel( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubModesSwitchToDelaySet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubModesNotificationNotifyAllSet( const gason::JsonValue& json,
                                                             common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubModesNotificationsSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubModesNotificationAdd( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubModesNotificationRemove( const gason::JsonValue& json,
                                                       common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

#ifdef CONFIG_SPIRAM
    virtual ResponceType onHubBridgeControllerConnectionSet( const gason::JsonValue& json,
                                                             common::JsonStreamGenerator& jsonGenerator ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubBridgeControllerConnectionList( const gason::JsonValue& json,
                                                              common::JsonStreamGenerator& jsonGenerator ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }
#endif  // CONFIG_SPIRAM

    virtual ResponceType onHubModesDisarmedDefaultSet( const gason::JsonValue& json,
                                                       common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubModesDisarmedDevicesAdd( const gason::JsonValue& json,
                                                       common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubModesDisarmedDevicesRemove( const gason::JsonValue& json,
                                                          common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubProvisionInfoSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubModesAlarmsOffAdd( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubModesAlarmsOffRemove( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

#ifndef CONFIG_UART_API
    virtual ResponceType onHubNetworkWifiTryConnect( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubNetworkEnabledSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubNetworkGet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubNetworkReset( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubNetworkWifiScanStart( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubNetworkWifiScanStop( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }
#endif  // not CONFIG_UART_API

    virtual ResponceType onHubReset( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubLogRemoteSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubLogLocalSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubRoomAllDelete( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubRoomCreate( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubRoomDelete( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubRoomGet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubRoomList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubRoomNameSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubRoomOrderSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubRoomViewSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubScenesNotificationAdd( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubScenesNotificationRemove( const gason::JsonValue& json,
                                                        common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubScenesRoomSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubTimeLocationGet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubTimeLocationSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubTimeLocationList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubReboot( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubScenesActionBlockTest( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubDevicesServiceNotificationsSet( const gason::JsonValue& json,
                                                              common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubSoftwareInfoGet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onRegisterNmaClient( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onRegistered( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubDeviceFirmwareUpdate( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    virtual ResponceType onHubOfflineLoginUi( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

    ResponceType onHubOfflineLoginHub( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        return std::make_tuple( common::resources::errors::MethodNotFound, StringType{} );
    }

public:
    inline void changeHandler( UPtr&& newHandler ) {
        m_newHandler = std::move( newHandler );
    }

    inline bool isNewHandler() const {
        return m_newHandler.operator bool();
    }

    inline auto& getNewHandler() {
        return m_newHandler;
    }

private:
    unsigned int m_msgId;

    UPtr m_newHandler;
};

}  // namespace nma
