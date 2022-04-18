#include <IDInfoManager.h>
#include <IDInfo.h>
#include <ProvisionDataDownloader.h>
#include <cstdio>
#include <DebugUtils.h>

namespace core {

namespace provision {

SyncRes syncProvisionData() {
    FUNC_LOGI( "Start syncing provision data" );

    auto& idInfo = core::IDInfo::get();
    constexpr auto buff_size = 64;
    char buff[buff_size];
    snprintf( buff, buff_size, SYNC_API_CALL_FORMAT, idInfo.configRevision() );
    std::string url{ idInfo.provisioningServerURL() };
    if( url.back() != '/' ) {
        url.append( "/" );
    }
    url.append( buff );
    ProvisionData provisionData;
    const auto syncRes = ProvisionDataDownloader::download( url, provisionData );

    if( SyncRes::Failed == syncRes ) {
        return SyncRes::Failed;
    }
    else if( SyncRes::NoChanges == syncRes ) {
        return SyncRes::NoChanges;
    }

    int new_config_rev;
    if( updateProvisionData( provisionData, new_config_rev ) ) {
        FUNC_LOGI( "Provision data were successfully updated" );
        idInfo.init();
        return SyncRes::Updated;
    }

    FUNC_LOGW( "Provision data were not updated" );
    return SyncRes::NoChanges;
}

bool validateProvisionData( const ProvisionData& data ) {
    return data.configRevision > 0 && !data.provisionServer.empty() && !data.provisionToken.empty() &&
           !data.cloudServer.empty() && !data.sslPrivateKey.empty() && !data.sslSharedKey.empty() &&
           !data.signingCaCertificate.empty();
    // && data.provisionServer.back() == '/' && data.cloudServer.back() != '/'; Hot fix EATT-2342 TODO: uncomment here
    // in future.
}

bool updateProvisionData( const ProvisionData& data, int& config_rev ) {
    FUNC_LOGI( "Start update provision data" );

    auto& idInfo = core::IDInfo::get();
    config_rev = idInfo.configRevision();
    bool res;

    if( validateProvisionData( data ) ) {
        FUNC_LOGI( "Update provision data to rev %d", data.configRevision );
        if( data.configRevision > idInfo.configRevision() ) {
            int new_rev = idInfo.writeProvisionData( data );
            if( new_rev > 0 ) {
                config_rev = new_rev;
                res = true;
            }
            else {
                FUNC_LOGE( "Failed writing provision data" );
                res = false;
            }
        }
        else {
            FUNC_LOGW( "The config's revision of downloaded data is (%d). Current is (%d) ",
                       data.configRevision,
                       idInfo.configRevision() );
            res = false;
        }
    }
    else {
        FUNC_LOGE( "Failed validating provision data" );
        res = false;
    }
    return res;
}

}  // namespace provision

}  // namespace core
