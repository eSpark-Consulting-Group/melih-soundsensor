
#pragma once

#include "NmaJsonHandlerBase.h"
#include "NmaBase.h"

#include <vector>
#include <memory>

namespace gateways {
class IGatewayUserDataStorage;
}
namespace core {
class EspLogger;
namespace features {
class FeatureFlagController;
}
}  // namespace core

namespace gason {
class JsonValue;
class JsonAllocator;
}  // namespace gason

namespace nma {

class WebSocketHandler : public NmaJsonHandlerBase {
public:
    WebSocketHandler( NmaBase& client, std::shared_ptr<::core::EspLogger>, ::gateways::IGatewayUserDataStorage& );

public:
    ResponceType onCloudTimeZonesGet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onExtensionsPluginInfoInstalled( const gason::JsonValue& json,
                                                  common::JsonStreamGenerator& gen ) override;

    ResponceType onExtensionsPluginInstallExecute( const gason::JsonValue& json,
                                                   common::JsonStreamGenerator& gen ) override;

    ResponceType onExtensionsPluginInstallRequest( const gason::JsonValue& json,
                                                   common::JsonStreamGenerator& gen ) override;

    ResponceType onExtensionsPluginUninstallExecute( const gason::JsonValue& json,
                                                     common::JsonStreamGenerator& gen ) override;

    ResponceType onExtensionsPluginUninstallRequest( const gason::JsonValue& json,
                                                     common::JsonStreamGenerator& gen ) override;

    ResponceType onHubAddonsZwaveNodesProvisioningAdd( const gason::JsonValue& json,
                                                       common::JsonStreamGenerator& gen ) override;

    ResponceType onHubAddonsZwaveNodesProvisioningRemove( const gason::JsonValue& json,
                                                          common::JsonStreamGenerator& gen ) override;

    ResponceType onHubAddonsZwaveNodesProvisioningSet( const gason::JsonValue& json,
                                                       common::JsonStreamGenerator& gen ) override;

    ResponceType onHubAddonsZwaveNodesProvisioningList( const gason::JsonValue& json,
                                                        common::JsonStreamGenerator& gen ) override;

    ResponceType onHubExtensionsPluginRun( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubCoordinatesGet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubInfoGet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubCoordinatesSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubFavoriteList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubFavoriteSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubFeaturesList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubFeatureStatusSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubGatewaysList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubDeviceForceRemove( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubDeviceArmedSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubServerRequest( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubScenesCreate( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubGlobalScenesCreate( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubScenesEdit( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubScenesGlobalEdit( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubSceneDelete( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubSceneGlobalDelete( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubScenesNameSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubScenesHouseModesSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubScenesEnabledSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubScenesGlobalEnabledSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubScenesList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubScenesGlobalList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubScenesBlocksList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubScenesTriggerDevicesList( common::JsonStreamGenerator& gen ) override;

    ResponceType onHubScenesRun( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubScenesGlobalRun( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubScenesGet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubScenesTimeList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubDataListRequest( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubDevicesList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
    ResponceType onHubDetectionDeviceAdd( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubDetectionDevicesList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubDetectionDeviceSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubDetectionDeviceRemove( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubDetectionDevicesScanStart( const gason::JsonValue& json,
                                                 common::JsonStreamGenerator& gen ) override;

    ResponceType onHubDetectionDevicesScanStop( const gason::JsonValue& json,
                                                common::JsonStreamGenerator& gen ) override;
#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

    ResponceType onHubDeviceSettingDictionaryValueSet( const gason::JsonValue& json,
                                                       common::JsonStreamGenerator& gen ) override;

    ResponceType onHubDeviceNameSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubDeviceRoomSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubItemsList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubSettingsListRequest( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubSettingValueSetRequest( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubSettingValueResetRequest( const gason::JsonValue& json,
                                                common::JsonStreamGenerator& gen ) override;

    ResponceType onHubDeviceSettingsList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubGatewaySettingsList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubItemDictionaryClear( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubItemDictionaryValueAdd( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubItemDictionaryValueSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubItemDictionaryValueRemove( const gason::JsonValue& json,
                                                 common::JsonStreamGenerator& gen ) override;

    ResponceType onHubItemValueSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubDeviceSettingValueSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubGatewaySettingValueSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubDeviceSettingReset( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubItemValuePercentApply( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubModesGet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubModesCurrentGet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubModesAlarmDelaySet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubModesSwitch( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubModesSwitchCancel( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubModesSwitchToDelaySet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubModesNotificationNotifyAllSet( const gason::JsonValue& json,
                                                     common::JsonStreamGenerator& gen ) override;

    ResponceType onHubModesNotificationsSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubModesNotificationAdd( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubModesNotificationRemove( const gason::JsonValue& json,
                                               common::JsonStreamGenerator& gen ) override;
#ifdef CONFIG_SPIRAM
    ResponceType onHubBridgeControllerConnectionSet( const gason::JsonValue& json,
                                                     common::JsonStreamGenerator& jsonGenerator ) override;

    ResponceType onHubBridgeControllerConnectionList( const gason::JsonValue& json,
                                                      common::JsonStreamGenerator& jsonGenerator ) override;
#endif  // CONFIG_SPIRAM

    ResponceType onHubModesDisarmedDefaultSet( const gason::JsonValue& json,
                                               common::JsonStreamGenerator& gen ) override;

    ResponceType onHubModesDisarmedDevicesAdd( const gason::JsonValue& json,
                                               common::JsonStreamGenerator& gen ) override;

    ResponceType onHubModesDisarmedDevicesRemove( const gason::JsonValue& json,
                                                  common::JsonStreamGenerator& gen ) override;

    ResponceType onHubProvisionInfoSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubModesAlarmsOffAdd( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubModesAlarmsOffRemove( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

#ifndef CONFIG_UART_API
    ResponceType onHubNetworkWifiTryConnect( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubNetworkEnabledSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubNetworkGet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubNetworkReset( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;
#endif  // not CONFIG_UART_API

    ResponceType onHubReset( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubLogRemoteSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubLogLocalSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubRoomAllDelete( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubRoomCreate( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubRoomDelete( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubRoomGet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubRoomList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubRoomNameSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubTimeLocationGet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubTimeLocationSet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubTimeLocationList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubReboot( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubScenesActionBlockTest( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubDevicesServiceNotificationsSet( const gason::JsonValue& json,
                                                      common::JsonStreamGenerator& gen ) override;

    ResponceType onHubSoftwareInfoGet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubDeviceFirmwareUpdate( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

private:
    ResponceType onHubScenesEnabledSetImpl( const char* busName,
                                            const gason::JsonValue& json,
                                            common::JsonStreamGenerator& gen );

    ResponceType onHubSceneDeleteImpl( const char* busName,
                                       const gason::JsonValue& json,
                                       common::JsonStreamGenerator& gen );

    ResponceType onHubScenesListImpl( const char* busName,
                                      const gason::JsonValue& json,
                                      common::JsonStreamGenerator& gen );

    ResponceType onHubScenesRunImpl( const char* busName,
                                     const gason::JsonValue& json,
                                     common::JsonStreamGenerator& gen );

    void commandResponseCloud( const char* comId, const char* res, const char* err );

    ResponceType onHubSceneRunProgressBroadcast( const gason::JsonValue& json,
                                                 common::JsonStreamGenerator& gen ) override;

protected:
    NmaBase& m_client;

private:
    std::shared_ptr<::core::EspLogger> m_espLogger;
    ::gateways::IGatewayUserDataStorage& m_gatewayUserDataStorage;
};

}  // namespace nma
