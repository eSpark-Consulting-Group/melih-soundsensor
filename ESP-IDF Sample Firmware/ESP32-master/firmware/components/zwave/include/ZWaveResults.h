
#pragma once

namespace gateways {
namespace zwave {

enum class Results {
    ReadyToExecute = 0,
    Waiting,
    OK,
    Failed,
    NotSupported,
    Undefined,
    WrongParameters,

    Last
};

const char* toString( Results status );

}  // namespace zwave
}  // namespace gateways
