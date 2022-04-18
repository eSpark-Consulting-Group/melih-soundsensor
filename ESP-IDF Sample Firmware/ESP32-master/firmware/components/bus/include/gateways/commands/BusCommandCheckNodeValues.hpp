#pragma once

#include "BusCommand.h"
#include <values/ArrayValue.h>
#include <chrono>

namespace bus::command {

/**
 * Request node values from specified command classes.
 */
class CheckNodeValues : public IBusCommand {
public:
    CheckNodeValues( uint8_t node_id,
                     std::chrono::milliseconds request_delay,
                     std::unique_ptr<::common::ArrayValue> cc_to_check )
    : m_node_id( node_id )
    , m_request_delay( request_delay )
    , m_cc_to_check( std::move( cc_to_check ) ) {}

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline const auto& node_id() const {
        return m_node_id;
    }

    inline const auto& request_delay() const {
        return m_request_delay;
    }

    inline const auto& cc_to_check() const {
        return *m_cc_to_check;
    }

private:
    uint8_t m_node_id;
    std::chrono::milliseconds m_request_delay;
    std::unique_ptr<::common::ArrayValue> m_cc_to_check;
};

}  // namespace bus::command
