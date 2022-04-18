#pragma once

#include "sdkconfig.h"

#ifndef CONFIG_UART_API

#include "NmaWebSocketHandler.h"

namespace cloud_client {

class NmaJsonHandler : public nma::WebSocketHandler {
public:
    using nma::WebSocketHandler::WebSocketHandler;

private:  // nma::WebSocketHandler
    ResponceType onRegistered( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onCloudFirmwareInfoGet( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubFirmwareUpdate( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubOfflineCredentialsUpdated( const gason::JsonValue& json,
                                                 common::JsonStreamGenerator& gen ) override;

    ResponceType onHubSoftwareUpdateExecute( const gason::JsonValue& json,
                                             common::JsonStreamGenerator& jsonGenerator ) override;

    ResponceType onHubNetworkWifiScanStart( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onHubNetworkWifiScanStop( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

#ifdef CONFIG_CLOUD_DEVICES
    ResponceType onCloudControllerAbstractSetResponse( const gason::JsonValue& json,
                                                       common::JsonStreamGenerator& gen ) override;

    ResponceType onCloudAbstractsDeleted( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onCloudAbstractsAdded( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onCloudAbstractsUpdated( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;

    ResponceType onCloudStorageControllerToken( const gason::JsonValue& json,
                                                common::JsonStreamGenerator& gen ) override;

    std::vector<std::string> m_device_ids;
#endif  // CONFIG_CLOUD_DEVICES

#ifdef CONFIG_LOCAL_MODE
    ResponceType onCloudAccessKeysControllerSync( const gason::JsonValue& json,
                                                  common::JsonStreamGenerator& gen ) override;
#endif  // CONFIG_LOCAL_MODE
};

}  // namespace cloud_client

#endif  // not CONFIG_UART_API
