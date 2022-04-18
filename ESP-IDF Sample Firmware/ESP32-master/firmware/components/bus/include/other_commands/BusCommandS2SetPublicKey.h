#pragma once

#include <BusCommand.h>

namespace bus {
namespace command {

struct S2SetPublicKey final : public IBusCommand {
    S2SetPublicKey()
    : m_key( 0 ) {}

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    uint32_t m_key;
};

}  // namespace command
}  // namespace bus
