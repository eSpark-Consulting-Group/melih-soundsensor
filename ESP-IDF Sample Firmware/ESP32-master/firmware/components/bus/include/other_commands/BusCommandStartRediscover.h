#pragma once

#include <string>

#include "BusCommand.h"

namespace bus {
namespace command {

class StartRediscover : public IBusCommand {
public:
    StartRediscover( uint8_t nodeId, std::string filepath )
    : m_nodeId( nodeId )
    , m_deviceSettingsFilepath( std::move( filepath ) ) {}

    std::string getDeviceSettingsFilepath() const {
        return m_deviceSettingsFilepath;
    }

    uint8_t getNodeId() const {
        return m_nodeId;
    }

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

private:
    uint8_t m_nodeId;
    std::string m_deviceSettingsFilepath;
};

}  // namespace command
}  // namespace bus
