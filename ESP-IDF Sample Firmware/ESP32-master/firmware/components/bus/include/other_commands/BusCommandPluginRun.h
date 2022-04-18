#pragma once

#include "BusCommand.h"
#include "gason.hpp"
#include "IValue.h"

#include <string>
#include <vector>
#include <memory>

namespace bus {
namespace command {

struct PluginRun final : public IBusCommand {
    using StringType = std::string;

    PluginRun( StringType&& _scriptName, StringType&& _scriptCode, common::IValue::UPtr _scriptParams )
    : scriptName( std::move( _scriptName ) )
    , scriptCode( std::move( _scriptCode ) )
    , scriptParams( std::move( _scriptParams ) ) {}

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) {
        client.visit( from, to, *this );
    }

    StringType scriptName;
    StringType scriptCode;
    common::IValue::UPtr scriptParams;
};

}  // namespace command
}  // namespace bus
