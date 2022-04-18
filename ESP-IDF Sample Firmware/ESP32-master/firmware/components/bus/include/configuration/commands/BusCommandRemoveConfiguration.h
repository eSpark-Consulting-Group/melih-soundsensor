#pragma once

#include <BusCommand.h>
#include <memory>

namespace configuration {
namespace commands {

class RemoveConfiguration final : public bus::IBusCommand {
public:
    RemoveConfiguration( uint8_t nodeId, uint16_t param_number )
    : m_nodeId( nodeId )
    , m_param_number( param_number ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline uint8_t nodeId() const {
        return m_nodeId;
    }

    auto paramNumber() const {
        return m_param_number;
    }

private:
    uint8_t m_nodeId;
    uint16_t m_param_number;
};

}  // namespace commands
}  // namespace configuration
