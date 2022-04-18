#include <memory>
#include <string>
#include <optional>

#include <znet/znet.h>
#include <znet/znet_error_message.hpp>

namespace common {
class IValue;
}

namespace bus::command {
class SendGenericEventToLuaHandler;
}

namespace bus::lua_event::impl {

std::unique_ptr<bus::command::SendGenericEventToLuaHandler> updateValueEvent( const char* eventName,
                                                                              const char* valueFieldName,
                                                                              std::string handlerName,
                                                                              znet_node_id_t nodeId,
                                                                              uint8_t channelId,
                                                                              uint8_t commandClassId,
                                                                              std::unique_ptr<::common::IValue> value );

std::unique_ptr<bus::command::SendGenericEventToLuaHandler> updateFloatValueEvent(
    const char* eventName,
    const char* typeFieldName,
    std::string handlerName,
    znet_node_id_t nodeId,
    uint8_t channelId,
    uint8_t classId,
    std::unique_ptr<::common::IValue> type,
    std::unique_ptr<::common::IValue> scale,
    std::optional<uint8_t> precision,
    float value );

std::unique_ptr<bus::command::SendGenericEventToLuaHandler> deviceListOperationProgress(
    const char* eventName,
    std::string handlerName,
    znet_node_id_t devicesAmount,
    znet_node_id_t currentDeviceIndex );

std::unique_ptr<bus::command::SendGenericEventToLuaHandler> entryLockSchedule(
    const char* eventName,
    std::string handlerName,
    znet_node_id_t nodeId,
    uint8_t channelId,
    uint8_t userId,
    uint8_t slotId,
    std::unique_ptr<::common::IValue> value );

std::unique_ptr<bus::command::SendGenericEventToLuaHandler> userCodeV2Event( const char* eventName,
                                                                             std::string handlerName,
                                                                             znet_node_id_t nodeId,
                                                                             uint8_t channelId,
                                                                             std::string value );

std::unique_ptr<::common::IValue> errorMessageToValue(::znet::ErrorOpt );

}  // namespace bus::lua_event::impl
