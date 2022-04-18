#pragma once

#include <GatewayDefs.h>
#include <GatewayID.h>
#include <PagesManager.h>
#include <NmaErrors.h>
#include <plugin_info.h>
#include <FeatureFlag.hpp>

#include <memory>

namespace common {
class JsonStreamGenerator;
class TableValue;
}  // namespace common

namespace znet {
struct ProvisioningListEntry;
struct ProvisioningListEntryFields;
}  // namespace znet

namespace nma {

class BroadcastsSender final {
private:
    static bool sendS2SendKey( const char* event, const uint16_t key[], size_t keySize );

public:
    enum UpdatingStatus { Started, Process, Finished, Aborted };

    /// INFO: bitmask flags for S2AuthMode
    static constexpr uint8_t S2AuthModeS2 = 0x01;
    static constexpr uint8_t S2AuthModeS2Authenticated = 0x02;
    static constexpr uint8_t S2AuthModeS2Unauthenticated = 0x04;
    static constexpr uint8_t S2AuthModeS0 = 0x08;
    static constexpr uint8_t S2AuthModeCSA = 0x10;

public:
    static bool writeDefaultHeader( common::JsonStreamGenerator& jsonGenerator,
                                    const char* broadcastName,
                                    const char* operationId = nullptr );

    static bool sendDescriptionEdited( const char* changedValue );

    static bool sendUpdatingFirmware( const uint8_t completed, uint32_t operationId, UpdatingStatus statusId );

    static bool sendUpdatingFirmwareError( Errors error, uint32_t operationId );

    static bool sendFoDevicePageUpdated( gateways::GatewayID gatewayID,
                                         const gateways::DeviceId& deviceId,
                                         const core::PagesManager::PageId& pageId );

    static bool sendFeatureStatusUpdate( const ::core::features::FeatureFlag& );

    static bool sendSettingStatusUpdateToken( const std::string& name, const std::string& value );
    static bool sendSettingStatusUpdateFlag( const std::string& name, bool value );
    static bool sendSettingStatusUpdateDictionary( const std::string& name, const std::string& key, const int value );

    static void sendProvisioningListEntryAdded( const ::znet::ProvisioningListEntry& );
    static void sendProvisioningListEntryUpdated( uint32_t entryId, const ::znet::ProvisioningListEntryFields& );
    static void sendProvisioningListEntryRemoved( uint32_t entryId );

    static void sendSettingDictionaryStatusUpdated( const std::string& settingId,
                                                    const std::string& elementId,
                                                    const std::string& status );

    static void sendGatewayUpdated(::gateways::GatewayID, std::unique_ptr<::common::TableValue>&& updatedFields );
};

}  // namespace nma
