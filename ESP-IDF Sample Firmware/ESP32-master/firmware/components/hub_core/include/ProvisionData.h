#pragma once
#include <string>
#include <vector>

namespace core {

struct ProvisionData {
    int configRevision;
    std::string provisionServer;
    std::string provisionToken;
    std::string cloudServer;
    std::string sslPrivateKey;
    std::string sslSharedKey;
    std::string signingCaCertificate;
    std::vector<std::string> timeServers;
    std::string productName;
};

namespace provision {
enum class SyncRes { Updated, NoChanges, Failed };
}  // namespace provision

}  // namespace core
