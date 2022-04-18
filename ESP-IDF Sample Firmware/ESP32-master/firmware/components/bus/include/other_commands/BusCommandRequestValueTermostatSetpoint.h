#pragma once

#include <BusCommandRequestValue.h>

namespace bus {
namespace command {

struct RequestValueTermostatSetpoint final : public RequestValue {
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    unsigned int m_setpointType;
};

}  // namespace command
}  // namespace bus
