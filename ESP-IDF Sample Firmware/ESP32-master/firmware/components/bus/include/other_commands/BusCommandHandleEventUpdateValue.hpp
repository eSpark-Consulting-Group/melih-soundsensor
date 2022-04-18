#pragma once

#include <memory>

#include <IValue.h>
#include "BusCommandHandleEvent.h"

#include <znet/optional.hpp>

namespace bus {
namespace command {

class HandleEventUpdateValue : public HandleEvent {
public:
    HandleEventUpdateValue( uint8_t commandClassId,
                            uint8_t channelId,
                            std::unique_ptr<::common::IValue> value,
                            std::string handlerName,
                            std::string event,
                            uint8_t nodeId )
    : HandleEvent( std::move( handlerName ), std::move( event ), nodeId )
    , m_commandClassId( commandClassId )
    , m_channelId( channelId )
    , m_value( std::move( value ) )
    , m_targetValue(::znet::utils::optional<uint8_t>() )
    , m_targetDuration(::znet::utils::optional<std::chrono::seconds>() ) {}

    HandleEventUpdateValue( uint8_t commandClassId,
                            uint8_t channelId,
                            std::unique_ptr<::common::IValue> value,
                            ::znet::utils::optional<uint8_t> targetValue,
                            std::string handlerName,
                            std::string event,
                            uint8_t nodeId,
                            ::znet::utils::optional<std::chrono::seconds> targetDuration )
    : HandleEvent( std::move( handlerName ), std::move( event ), nodeId )
    , m_commandClassId( commandClassId )
    , m_channelId( channelId )
    , m_value( std::move( value ) )
    , m_targetValue( targetValue )
    , m_targetDuration( targetDuration ) {}

    uint8_t commandClassId() const {
        return m_commandClassId;
    }

    uint8_t channelId() const {
        return m_channelId;
    }

    const ::common::IValue* value() {
        return m_value.get();
    }

    ::znet::utils::optional<uint8_t> targetValue() {
        return m_targetValue;
    }

    ::znet::utils::optional<std::chrono::seconds> targetDuration() {
        return m_targetDuration;
    }

private:
    uint8_t m_commandClassId;
    uint8_t m_channelId;
    common::IValue::UPtr m_value;
    ::znet::utils::optional<uint8_t> m_targetValue;
    ::znet::utils::optional<std::chrono::seconds> m_targetDuration;
};

}  // namespace command
}  // namespace bus
