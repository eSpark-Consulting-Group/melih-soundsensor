#pragma once

#include <string>

#include "BusCommand.h"

namespace bus {
namespace command {

class StartInclude : public IBusCommand {
public:
    StartInclude( std::string filepath )
    : m_deviceSettingsFilepath( std::move( filepath ) ) {}

    StartInclude() = default;

    std::string getDeviceSettingsFilepath() const {
        return m_deviceSettingsFilepath;
    }

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

private:
    std::string m_deviceSettingsFilepath;
};

}  // namespace command
}  // namespace bus
