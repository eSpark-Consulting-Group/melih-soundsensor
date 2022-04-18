#pragma once

#include <BusCommand.h>
#include <string>

namespace bus {
namespace command {

class LockAntiTheft final : public bus::IBusCommand {
public:
    LockAntiTheft( uint8_t nodeId, uint8_t channelId, std::string magicCode, std::string hint )
    : m_nodeId( nodeId )
    , m_channelId( channelId )
    , m_magic_code( magicCode )
    , m_hint( hint ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline uint8_t nodeId() const {
        return m_nodeId;
    }

    inline uint8_t channelId() const {
        return m_channelId;
    }

    inline std::string magicCode() const {
        return m_magic_code;
    }

    inline std::string hint() const {
        return m_hint;
    }

private:
    uint8_t m_nodeId;
    uint8_t m_channelId;
    std::string m_magic_code;
    std::string m_hint;
};

}  // namespace command
}  // namespace bus
