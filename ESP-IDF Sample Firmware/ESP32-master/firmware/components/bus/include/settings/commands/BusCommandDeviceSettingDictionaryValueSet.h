#pragma once

#include <BusCommand.h>
#include <string>
#include <IValue.h>
#include <list>

namespace settings {
namespace commands {

class DeviceSettingDictionaryValueSet final : public bus::IBusCommand {
public:
    using StringType = std::string;

    DeviceSettingDictionaryValueSet( StringType&& settingId, common::IValue::UPtr&& value, StringType&& key )
    : m_settingId( std::move( settingId ) )
    , m_value( std::move( value ) )
    , m_key( std::move( key ) ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline const auto& settingId() const {
        return m_settingId;
    }

    inline const auto& value() const {
        return m_value;
    }

    inline const auto& key() const {
        return m_key;
    }

private:
    StringType m_settingId;
    common::IValue::UPtr m_value;
    StringType m_key;
};

}  // namespace commands
}  // namespace settings
