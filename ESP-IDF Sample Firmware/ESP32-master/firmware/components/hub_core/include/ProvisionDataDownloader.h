#pragma once
#include <ProvisionData.h>

namespace core {

class ProvisionDataDownloader {
public:
    static constexpr auto BUFFER_SIZE = 4096u;
    static constexpr auto AUTH_KEY = "controller-key";
    static provision::SyncRes download( const std::string& addr, ProvisionData& provisionData );

private:
    static bool parseProvisionData( char* buff, ProvisionData& provisionData );
};

}  // namespace core
