#pragma once

#include <BusCommand.h>
#include <string>
#include <gason.hpp>
#include <IValue.h>
#include <list>

#include <DebugUtils.h>

namespace bus {

class IBusClient;

namespace command {

struct CloudItemValueSet final : public IBusCommand {
    using ListOfItemId = std::list<std::string>;

    CloudItemValueSet( const std::string& _pluginName, const ListOfItemId& _items, const common::IValue::UPtr& _value )
    : pluginName( _pluginName )
    , items( _items )
    , value( _value->clone() ) {}

    CloudItemValueSet( const std::string& _pluginName, const std::string& _item, const common::IValue::UPtr& _value )
    : pluginName( _pluginName )
    , items( 1, _item )
    , value( _value->clone() ) {}

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    static bool send( const char* receiver,
                      const IBusClient& client,
                      ListOfItemId&& items,
                      common::IValue::UPtr&& value );

    std::string pluginName;
    ListOfItemId items;
    common::IValue::UPtr value;
};

}  // namespace command
}  // namespace bus
