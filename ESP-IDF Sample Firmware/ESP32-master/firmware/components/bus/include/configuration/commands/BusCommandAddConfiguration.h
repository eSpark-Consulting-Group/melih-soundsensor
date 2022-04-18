#pragma once

#include <BusCommand.h>

namespace configuration {
namespace commands {

class AddConfiguration final : public bus::IBusCommand {
public:
    AddConfiguration( uint8_t nodeId, uint16_t param_number, uint8_t param_size )
    : m_nodeId( nodeId )
    , m_param_number( param_number )
    , m_param_size( param_size ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline uint8_t nodeId() const {
        return m_nodeId;
    }

    auto paramNumber() const {
        return m_param_number;
    }

    inline uint8_t paramSize() const {
        return m_param_size;
    }

private:
    uint8_t m_nodeId;
    uint16_t m_param_number;
    uint8_t m_param_size;
};

}  // namespace commands
}  // namespace configuration
