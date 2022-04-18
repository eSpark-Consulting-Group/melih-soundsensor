#pragma once

#include "BusCommand.h"

namespace bus::command {

/**
 * Send command for checking if device is online
 */
class CheckReachableStatus : public IBusCommand {
public:
    CheckReachableStatus( uint8_t node_id )
    : m_node_id( node_id ) {}

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline const auto& node_id() const {
        return m_node_id;
    }

private:
    uint8_t m_node_id;
};

}  // namespace bus::command
