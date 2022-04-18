#pragma once

#include <BusCommand.h>
#include <string>
#include <IValue.h>
#include <list>

namespace settings {
namespace commands {

class GatewaySettingValueSet final : public bus::IBusCommand {
public:
    using StringType = std::string;

    GatewaySettingValueSet( StringType&& settingId, common::IValue::UPtr&& value )
    : m_settingId( std::move( settingId ) )
    , m_value( std::move( value ) ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline const auto& settingId() const {
        return m_settingId;
    }

    inline const auto& value() const {
        return m_value;
    }

private:
    StringType m_settingId;
    common::IValue::UPtr m_value;
};

}  // namespace commands
}  // namespace settings
