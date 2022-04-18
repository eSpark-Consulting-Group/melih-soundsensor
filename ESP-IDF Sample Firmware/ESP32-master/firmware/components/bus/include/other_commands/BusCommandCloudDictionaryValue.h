#pragma once

#include <string>
#include <memory>

#include "BusCommand.h"

namespace common {
class IValue;
}

namespace bus {
namespace command {

class CloudDictionaryValue final : public IBusCommand {
public:
    enum DictionaryCommand {
        Add = 0x00,
        Set = 0x01,
        Remove = 0x02,
        Clear = 0x03,
    };

    CloudDictionaryValue( std::string pluginName, std::string itemId, std::string key, DictionaryCommand command )
    : m_pluginName( std::move( pluginName ) )
    , m_itemId( std::move( itemId ) )
    , m_key( key )
    , m_command( command ) {}

    CloudDictionaryValue( std::string pluginName,
                          std::string itemId,
                          std::unique_ptr<::common::IValue> value,
                          std::string valueType,
                          std::string key,
                          DictionaryCommand command )
    : m_pluginName( std::move( pluginName ) )
    , m_itemId( std::move( itemId ) )
    , m_value( std::move( value ) )
    , m_valueType( std::move( valueType ) )
    , m_key( key )
    , m_command( command ) {}

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    std::string pluginName() const {
        return m_pluginName;
    }

    std::string itemId() const {
        return m_itemId;
    }

    const std::unique_ptr<::common::IValue>& value() const {
        return m_value;
    }

    std::string valueType() const {
        return m_valueType;
    }

    std::string key() const {
        return m_key;
    }

    DictionaryCommand command() const {
        return m_command;
    }

private:
    std::string m_pluginName;
    std::string m_itemId;
    std::unique_ptr<::common::IValue> m_value;
    std::string m_valueType;
    std::string m_key;
    DictionaryCommand m_command;
};

}  // namespace command
}  // namespace bus
