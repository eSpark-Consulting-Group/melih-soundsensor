
#include "ZWaveResults.h"

#include "CommonResources.h"

namespace gateways {
namespace zwave {

const char* toString( Results status ) {
    switch( status ) {
    case Results::ReadyToExecute:
        return "ReadyToExecute";

    case Results::Waiting:
        return "Waiting";

    case Results::OK:
        return "OK!";

    case Results::Failed:
        return "Failed";

    case Results::NotSupported:
        return "NotSupported";

    case Results::Undefined:
        return "Undefined";

    default:
        break;
    };
    return common::resources::tokens::Unknown;
}

}  // namespace zwave
}  // namespace gateways
