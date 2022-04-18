#pragma once

#include "NmaBase.h"
#include "ISocket.h"
#include "NmaBase.h"

#include <memory>

namespace gateways {
class IGatewayUserDataStorage;
}
namespace core {
class EspLogger;
}  // namespace core

namespace local_tcp_srv {

class LocalTcpSrvNmaClient : public nma::NmaBase {
public:
    using UPtr = std::unique_ptr<LocalTcpSrvNmaClient>;

    LocalTcpSrvNmaClient( network::ISocket::UPtr&& sock,
                          std::shared_ptr<core::EspLogger>,
                          ::gateways::IGatewayUserDataStorage& );

    ~LocalTcpSrvNmaClient();
};

}  // namespace local_tcp_srv