#pragma once

#include "BusCommand.h"
#include "CommonResources.h"

#include <tuple>
#include <string>

namespace scenes {
namespace commands {

using BaseCmdResponceType = std::tuple<int, std::string>;
extern const BaseCmdResponceType ResponceCmdWasntHandled;

class Base : public bus::BusCommandResp<BaseCmdResponceType, ResponceCmdWasntHandled> {
public:
    using ResponceType = std::tuple<int, std::string>;
    using ParentType = bus::BusCommandResp<ResponceType, ResponceCmdWasntHandled>;

    using ParentType::setResponce;

    inline static ResponceType createExecTimeoutErrorResult() {
        return std::make_tuple( common::resources::errors::InternalError, "Command execution timeout" );
    }
};

}  // namespace commands
}  // namespace scenes
