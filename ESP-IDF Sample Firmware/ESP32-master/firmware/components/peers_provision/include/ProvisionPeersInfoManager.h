#pragma once

#include "gason.hpp"
#include "PeerInfo.h"
#include "Shutdownable.h"
#include "KeyValueStorageFactory.h"

#include <list>
#include <map>
#include <mutex>
#include <tuple>

namespace provision {
namespace peers {

class InfoManager : public core::SingletonBaseImpl<InfoManager> {
    friend class core::SingletonBaseImpl<InfoManager>;

public:
    using ParentType = core::SingletonBaseImpl<InfoManager>;
    using ClbkRet = ParentType::ClbkRet;
    using UpdContext = core::upd_srv::UpdContext;

    using PeerssInfoMap = std::map<std::string, PeerInfo>;
    using StorUPtr = storage::KeyValueStorageFactory::StoragePtr;
    using BlobType = storage::KeyValueStorage::BlobType;
    using Stringtype = std::string;

    ~InfoManager();

    bool sync_local_mode();
#ifdef CONFIG_LOCAL_MODE
    void pars_sync_local_mode( gason::JsonValue json );
#endif  // CONFIG_LOCAL_MODE
    inline void set_update( bool val ) {
        is_update = val;
    }

    inline bool get_update() const {
        return is_update;
    }

    inline auto get_version() const {
        return m_version;
    }

    char* getUsers() {
        return clients_db.data();
    }

    BlobType getTlsPublicCertificate();

    BlobType getTlsPrivateKey();

    BlobType getPublicParent();

    bool storePeersData( const gason::JsonValue& json );

    std::list<std::string> getTokens();

    bool addOrUpdatePeersIp( const std::string& uuid, std::list<std::string>& ip );

    std::list<PeerInfo> getPeersInfo();

    bool savePeers();
    bool loadPeers();

    bool saveClients( storage::KeyValueStorage& stor );
    bool loadClients( storage::KeyValueStorage& stor );

    ClbkRet updateComponent( UpdContext& ctx ) override;

    StorUPtr getStor() const;

protected:
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const cloud_client::responces::ConnectedToCloud& command ) override;

private:
    using ParentType::ParentType;

    bool initializeImpl() override;

private:
    std::int32_t m_version = 1;
    PeerssInfoMap m_Peers;

    BlobType clients_db;
    BlobType current_db;

    bool is_update = false;
};

};  // namespace peers
};  // namespace provision
