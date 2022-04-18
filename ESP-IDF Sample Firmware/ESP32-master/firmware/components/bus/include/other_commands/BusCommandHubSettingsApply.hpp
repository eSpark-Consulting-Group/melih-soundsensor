#pragma once

#include "BusCommand.h"
#include <string>

namespace bus {
namespace command {

class HubSettingsApply final : public IBusCommand {
public:
    HubSettingsApply( std::string name, std::unique_ptr<::common::IValue> value )
    : m_name( std::move( name ) )
    , m_value( std::move( value ) ) {}

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) {
        client.visit( from, to, *this );
    }

    std::string getName() const {
        return m_name;
    }

    ::common::IValue* getValue() const {
        return m_value.get();
    }

private:
    std::string m_name;
    std::unique_ptr<::common::IValue> m_value;
};

}  // namespace command
}  // namespace bus
