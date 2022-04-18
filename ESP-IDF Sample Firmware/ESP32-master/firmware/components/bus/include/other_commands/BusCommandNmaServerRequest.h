#pragma once
#include "BusCommand.h"

namespace bus {
namespace command {
namespace nma {

struct ServerRequest final : public IBusCommand {
    virtual ~ServerRequest() override = default;

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) {
        client.visit( from, to, *this );
    }
};

}  // namespace nma
}  // namespace command
}  // namespace bus