#pragma once

#include "BusCommand.h"

#include <vector>

namespace cloud_client {
namespace responces {

class ReceivedPeersProvision : public bus::IBusCommand {
public:
    using BlobType = std::vector<char>;

    ReceivedPeersProvision( BlobType pubCert, BlobType privKey )
    : m_pubCert( std::move( pubCert ) )
    , m_privKey( std::move( privKey ) ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    auto& pubCert() {
        return m_pubCert;
    }

    auto& privKey() {
        return m_privKey;
    }

private:
    BlobType m_pubCert;
    BlobType m_privKey;
};

}  // namespace responces
}  // namespace cloud_client
