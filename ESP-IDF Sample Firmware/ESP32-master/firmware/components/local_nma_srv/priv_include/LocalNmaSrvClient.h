#pragma once

#include "NmaBase.h"
#include "WebSocketServer.h"

namespace gateways {
class IGatewayUserDataStorage;
}
namespace core {
class EspLogger;
namespace features {
class FeatureFlagController;
}  // namespace features
}  // namespace core

namespace common {
class IReadStream;
}  // namespace common

namespace local_nma_srv {

class LocalNmaClient : public nma::NmaBase {
public:
    using nma::NmaBase::NmaBase;  // Using parent's constructors

private:
    bool initializeImpl() override;
};

}  // namespace local_nma_srv
