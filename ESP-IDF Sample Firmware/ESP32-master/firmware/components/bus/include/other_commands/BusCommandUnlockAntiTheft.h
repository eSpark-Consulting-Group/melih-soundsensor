#pragma once

#include <BusCommand.h>
#include <string>

namespace bus {
namespace command {

class UnlockAntiTheft final : public bus::IBusCommand {
public:
    UnlockAntiTheft( uint8_t nodeId, uint8_t channelId, uint8_t classId, std::string magicCode )
    : m_nodeId( nodeId )
    , m_channelId( channelId )
    , m_classId( classId )
    , m_magic_code( magicCode ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline uint8_t nodeId() const {
        return m_nodeId;
    }

    inline uint8_t channelId() const {
        return m_channelId;
    }

    inline uint8_t classId() const {
        return m_classId;
    }

    inline std::string magicCode() const {
        return m_magic_code;
    }

private:
    uint8_t m_nodeId;
    uint8_t m_channelId;
    uint8_t m_classId;
    std::string m_magic_code;
};

}  // namespace command
}  // namespace bus
