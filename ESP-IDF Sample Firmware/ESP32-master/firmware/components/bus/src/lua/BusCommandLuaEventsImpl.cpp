#include "lua/BusCommandLuaEventsImpl.h"

#include "IValue.h"
#include "CommonResources.h"

#include "lua/BusCommandSendEventToLuaHandler.hpp"

namespace lua_res = ::common::resources::lua_events;

namespace bus::lua_event::impl {

std::unique_ptr<bus::command::SendGenericEventToLuaHandler> updateValueEvent(
    const char* eventName,
    const char* valueFieldName,
    std::string handlerName,
    znet_node_id_t nodeId,
    uint8_t channelId,
    uint8_t commandClassId,
    std::unique_ptr<::common::IValue> value ) {
    using ::common::IValue;
    return std::unique_ptr<bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler( std::move( handlerName ),
                                                          eventName,
                                                          { { lua_res::NodeId, IValue::create( nodeId ) },
                                                            { lua_res::ChannelId, IValue::create( channelId ) },
                                                            { lua_res::ClassId, IValue::create( commandClassId ) },
                                                            { valueFieldName, std::move( value ) } } ) );
}

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
    float value ) {
    using ::common::IValue;
    return std::unique_ptr<bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler(
            std::move( handlerName ),
            eventName,
            { { lua_res::NodeId, IValue::create( nodeId ) },
              { lua_res::ClassId, IValue::create( classId ) },
              { lua_res::ChannelId, IValue::create( channelId ) },
              { lua_res::Scale, std::move( scale ) },
              { lua_res::Value, IValue::create( value ) },
              { lua_res::Precision, precision ? ::common::IValue::create( *precision ) : std::unique_ptr<IValue>{} },
              { typeFieldName, std::move( type ) } } ) );
}

std::unique_ptr<bus::command::SendGenericEventToLuaHandler> deviceListOperationProgress(
    const char* eventName,
    std::string handlerName,
    znet_node_id_t devicesAmount,
    znet_node_id_t currentDeviceIndex ) {
    using ::common::IValue;
    return std::unique_ptr<bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler(
            std::move( handlerName ),
            eventName,
            { { lua_res::Devices, IValue::create( devicesAmount ) },
              { lua_res::Current, IValue::create( currentDeviceIndex ) } } ) );
}

std::unique_ptr<bus::command::SendGenericEventToLuaHandler> entryLockSchedule(
    const char* eventName,
    std::string handlerName,
    znet_node_id_t nodeId,
    uint8_t channelId,
    uint8_t userId,
    uint8_t slotId,
    std::unique_ptr<::common::IValue> value ) {
    using ::common::IValue;
    return std::unique_ptr<bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler(
            std::move( handlerName ),
            eventName,
            { { lua_res::NodeId, IValue::create( nodeId ) },
              { lua_res::ClassId, IValue::create( ZNET_COMMAND_CLASS_SCHEDULE_ENTRY_LOCK ) },
              { lua_res::ChannelId, IValue::create( channelId ) },
              { lua_res::UserId, IValue::create( userId ) },
              { lua_res::SlotId, IValue::create( slotId ) },
              { lua_res::Value, std::move( value ) } } ) );
}

std::unique_ptr<bus::command::SendGenericEventToLuaHandler> userCodeV2Event( const char* eventName,
                                                                             std::string handlerName,
                                                                             znet_node_id_t nodeId,
                                                                             uint8_t channelId,
                                                                             std::string value ) {
    using ::common::IValue;
    return std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler(
            std::move( handlerName ),
            eventName,
            { { lua_res::NodeId, IValue::create( nodeId ) },
              { lua_res::ChannelId, IValue::create( channelId ) },
              { lua_res::ClassId, IValue::create( ZNET_COMMAND_CLASS_USER_CODE ) },
              { lua_res::Value, IValue::create( value ) } } ) );
}

std::unique_ptr<::common::IValue> errorMessageToValue(::znet::ErrorOpt errorMessage ) {
    std::unique_ptr<::common::TableValue> errorTable;
    using ::common::IValue;
    if( errorMessage ) {
        errorTable = std::make_unique<::common::TableValue>();
        errorTable->insert( lua_res::Code, IValue::create( errorMessage->m_code ) );
        errorTable->insert( lua_res::Data, IValue::create( std::move( errorMessage->m_data ) ) );
        if( !errorMessage->m_details.empty() ) {
            auto details = std::make_unique<::common::TableValue>();
            for( auto&& field : errorMessage->m_details ) {
                details->insert( field.first, IValue::create( std::move( field.second ) ) );
            }
            errorTable->insert( lua_res::Details, std::move( details ) );
        }
    }
    return errorTable;
}

}  // namespace bus::lua_event::impl
