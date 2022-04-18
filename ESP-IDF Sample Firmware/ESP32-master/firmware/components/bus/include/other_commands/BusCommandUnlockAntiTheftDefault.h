#pragma once

#include <BusCommand.h>
#include <string>

namespace bus {
namespace command {

class UnlockAntiTheftDefault final : public bus::IBusCommand {
public:
    UnlockAntiTheftDefault( uint8_t nodeId, uint8_t channelId, uint8_t classId, std::string hint )
    : m_nodeId( nodeId )
    , m_channelId( channelId )
    , m_classId( classId )
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

    inline uint8_t classId() const {
        return m_classId;
    }

    inline std::string hint() const {
        return m_hint;
    }

private:
    uint8_t m_nodeId;
    uint8_t m_channelId;
    uint8_t m_classId;
    std::string m_hint;
};

}  // namespace command
}  // namespace bus
