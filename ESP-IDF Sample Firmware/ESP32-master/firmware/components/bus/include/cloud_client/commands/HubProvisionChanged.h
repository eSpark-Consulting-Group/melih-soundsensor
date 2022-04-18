#pragma once

#include "BusCommand.h"

#include <string>

namespace cloud_client {
namespace commands {

class HubProvisionChanged : public bus::IBusCommand {
public:
    HubProvisionChanged( const std::string& nmaSrvName, const unsigned int nmaSrvPort, const bool isConnectionSecured )
    : m_nmaSrvName( nmaSrvName )
    , m_nmaSrvPort( nmaSrvPort )
    , m_isConnectionSecured( isConnectionSecured ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    const auto& nmaSrvName() const {
        return m_nmaSrvName;
    }

    const auto nmaSrvPort() const {
        return m_nmaSrvPort;
    }

    const auto isConnectionSecured() const {
        return m_isConnectionSecured;
    }

private:
    std::string m_nmaSrvName;
    unsigned int m_nmaSrvPort;
    bool m_isConnectionSecured;
};

}  // namespace commands
}  // namespace cloud_client
