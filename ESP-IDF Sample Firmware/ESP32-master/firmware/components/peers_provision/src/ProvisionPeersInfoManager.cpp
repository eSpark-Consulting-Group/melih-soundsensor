#include "ProvisionPeersInfoManager.h"

#include "CommonResources.h"
#include "BusCommandNmaClientClbk.h"
#include "HardUtils.h"
#include "Ezlo.h"
#include "JsonStreamGenerator.h"
#include "IValue.h"
#include "IRamBufStream.h"

#include "StorageSection.h"
#include "BinaryFileStorageFactory.h"
#include "cloud_client/responces/ReceivedPeersProvision.h"

#include <sstream>

#include "mbedtls/certs.h"

#include "DebugUtils.h"

namespace provision {
namespace peers {

namespace {

using BufferStreamType = common::IRamBufferStream;

char* json_tostr( gason::JsonValue json ) {
    auto client = json.toNode();
    char* tmp = (char*)malloc( 5 * 1024 );
    tmp[0] = 0;
    int len = 0;
    int index = 0;
    bool res = false;
    while( client ) {
        int len_key = strlen( client->key );

        sprintf( &tmp[index], "\"%s\"", client->key );
        index = index + len_key + 2;
        int i = 0;
        while( client->value[i] ) {
            char* tmp_v = client->value[i].toString( &res );
            if( res ) {
                int len_key = strlen( tmp_v );
                sprintf( &tmp[index], "\"%s\"", tmp_v );
                index = index + len_key + 2;
            }
            i++;
        }
        sprintf( &tmp[index], "\r\n" );
        index = index + 2;
        client = client->next;
    }
    len = strlen( tmp );
    char* out_str = (char*)malloc( len + 1 );
    memcpy( out_str, tmp, len );
    out_str[len] = 0;
    free( tmp );
    return out_str;
}

constexpr auto UUID_MARKER = "UUID";
constexpr auto TOKENES_MARKER = "tokens";
constexpr auto TOKENES_KEY = "tokens";
constexpr auto PEERS_DB_KEY = "peers_db";
constexpr auto UUID_KEY = "uuid";
constexpr auto IP_KEY = "ip";
constexpr auto SERIAL_KEY = "serial";
constexpr auto STATUS_KEY = "status";
constexpr auto END_DELIMITER = "\r\n";

static constexpr const char* PeersStorKey = "PeersStorKey";
}  // namespace

InfoManager::~InfoManager() {
    FUNC_LOGI( "InfoManager instance deleted" );
}

bool InfoManager::savePeers() {
    BufferStreamType stream( BufferStreamType::MiddleBufSize );

    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::LOCAL );

    bool is_successfull = stream && stream.write( m_Peers ) && stor->setBlob( PeersStorKey, stream, stream.size() );
    if( is_successfull ) {
        FUNC_LOGI( "Save peers info to storage - done" );
    }
    else {
        FUNC_LOGE( "Save peers info to storage - error" );
    }

    return is_successfull;
}

bool InfoManager::loadPeers() {
    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::LOCAL );
    storage::KeyValueStorage::BlobType blob;
    if( stor->getBlob( PeersStorKey, blob ) ) {
        common::ArrayRStream stream( blob.data(), blob.size() );
        if( stream.read( m_Peers ) ) {
        }
        else {
            FUNC_LOGE( "Can't load peers from storage!" );
            return false;
        }
    }
    else {
        FUNC_LOGW( "Can't get peers blob from storage!" );
    }

    return true;
}

bool InfoManager::initializeImpl() {
    if( !ParentType::initializeImpl() ) {
        FUNC_LOGE( "Can't initialize InfoManager!" );
        return false;
    }

    if( !bus::Bus::get().join( cloud_client::responces::BUS_NAME, *this ) ) {
        FUNC_LOGE( "Can't join to bus namespace: %s", cloud_client::responces::BUS_NAME );
        return false;
    }

    auto stor = getStor();
    if( !stor ) {
        return false;
    }

    if( !stor->getInt32( "version_lm", m_version ) ) {
        m_version = 1;
    }

    if( !stor->getBlob( "clients_db", clients_db ) ) {
        clients_db.push_back( '\n' );
    }

    if( !stor->getBlob( "current_db", current_db ) ) {
        current_db.push_back( '\n' );
    }

    return true;
}

InfoManager::BlobType InfoManager::getTlsPublicCertificate() {
    BlobType certificate_cur;

    auto stor = getStor();
    if( stor ) {
        stor->getBlob( "certificate_cur", certificate_cur );
    }

    return std::move( certificate_cur );
}

InfoManager::BlobType InfoManager::getTlsPrivateKey() {
    BlobType private_cur;

    auto stor = getStor();
    if( stor ) {
        stor->getBlob( "private_cur", private_cur );
    }

    return std::move( private_cur );
}

InfoManager::BlobType InfoManager::getPublicParent() {
    BlobType public_parent_cur;

    auto stor = getStor();
    if( !stor || !stor->getBlob( "public_parent_cur", public_parent_cur ) ) {
        public_parent_cur.resize( 1 );
        public_parent_cur[0] = '\n';
    }

    return std::move( public_parent_cur );
}

bool InfoManager::sync_local_mode() {
    auto clbk = [this]( common::IWriteStream& nmaStream ) -> bool {
        common::JsonStreamGenerator jsonGenerator( nmaStream );

        bool result = jsonGenerator.writeOpeningObject();
        result = result && jsonGenerator.writeProperty( common::resources::tokens::Id, core::Ezlo::uuid() );
        result = result && jsonGenerator.writeProperty( common::resources::tokens::Method,
                                                        common::resources::commands::CloudAccessKeysControllerSync );
        result = result && jsonGenerator.writeProperty( common::resources::tokens::Api, "2" );
        result = result && jsonGenerator.writePropertyKey( common::resources::tokens::Params );
        result = result && jsonGenerator.writeOpeningObject();
        if( result && m_version > 1 ) {
            result = jsonGenerator.writeProperty( "version", m_version );
        }
        result = result && jsonGenerator.writeClosingSymbol();

        result = result && jsonGenerator.writeJsonEnd();
        is_update = false;
        return result;
    };

    constexpr auto CmdTimeoutSec = std::chrono::seconds( 5 );
    return bus::waitToResp<bus::command::nma::ClientClbk>( "nma", CmdTimeoutSec, std::move( clbk ) );
}

#ifdef CONFIG_LOCAL_MODE
void InfoManager::pars_sync_local_mode( gason::JsonValue json ) {
    const char* requestId = json( common::resources::tokens::Id ).toString();
    bool res = false;
    auto error = json( common::resources::tokens::Error );
    if( !requestId ) {
        FUNC_LOGE( "No ID field in received message" );
        return;
    }
    if( strcasecmp( requestId, core::Ezlo::uuid() ) ) {
        FUNC_LOGE( "Not valid UUID in received message" );
        return;
    }

    if( error ) {
        int err_code = error( "code" ).toInt( &res );
        auto tt = error.toNode();
        if( res ) {
            char* data_err = error( "data" ).toString( &res );
            if( res ) {
                FUNC_LOGE( "Error code %d %s. Error json key %s", err_code, data_err, tt->key );
                return;
            }
        }
    }
    auto result_ver = json( common::resources::tokens::Result )( "version" ).toInt( &res );
    if( !res ) {
        FUNC_LOGE( "Not valid version in received message" );
        return;
    }

    auto modif = json( common::resources::tokens::Result )( "not_modified" ).toInt( &res );
    if( res ) {
        if( result_ver == m_version && modif ) {
            return;
        }
    }
    else if( m_version == result_ver ) {
        return;
    }

    auto stor = getStor();
    if( !stor ) {
        return;
    }

    m_version = result_ver;
    stor->setInt32( "version_lm", m_version );

    {
        const auto clientsCStr = json_tostr( json( common::resources::tokens::Result )( "clients" ) );
        if( clientsCStr ) {
            auto strLen = std::strlen( clientsCStr );
            if( 0 < strLen ) {
                clients_db.resize( strLen + 1 );
                std::memcpy( clients_db.data(), clientsCStr, strLen );
                clients_db[strLen] = '\n';
                stor->setBlob( "clients_db", clients_db );
            }
            else {
                clients_db.resize( 1 );
                clients_db[0] = '\n';
            }

            free( clientsCStr );
        }
    }

    storePeersData( json( common::resources::tokens::Result )( "peers" ) );

    auto current_tmp = json( common::resources::tokens::Result )( "current" )[0];
    if( current_tmp ) {
        current_db.resize( 2 );
        current_db[0] = '1';
        current_db[1] = '\n';
        stor->setBlob( "current_db", current_db );

        {
            BlobType certificate_cur;
            const auto certificate_cur_cStr = current_tmp( "certificate" ).toString();
            if( certificate_cur_cStr ) {
                auto strLen = std::strlen( certificate_cur_cStr );
                if( 0 < strLen ) {
                    certificate_cur.resize( strLen + 1 );
                    std::memcpy( certificate_cur.data(), certificate_cur_cStr, strLen );
                    certificate_cur[strLen] = '\n';
                    stor->setBlob( "certificate_cur", certificate_cur );
                }
            }

            BlobType private_cur;
            const auto private_cur_cStr = current_tmp( "private" ).toString();
            if( private_cur_cStr ) {
                auto strLen = std::strlen( private_cur_cStr );
                if( 0 < strLen ) {
                    private_cur.resize( strLen + 1 );
                    std::memcpy( private_cur.data(), private_cur_cStr, strLen );
                    private_cur[strLen] = '\n';
                    stor->setBlob( "private_cur", private_cur );
                }
            }

            send<cloud_client::responces::ReceivedPeersProvision>(
                cloud_client::responces::BUS_NAME, std::move( certificate_cur ), std::move( private_cur ) );
        }

        {
            const auto public_parent_cur_cStr = current_tmp( "public_parent" ).toString();
            if( public_parent_cur_cStr ) {
                auto strLen = std::strlen( public_parent_cur_cStr );
                if( 0 < strLen ) {
                    BlobType public_parent_cur;
                    public_parent_cur.resize( strLen + 1 );
                    std::memcpy( public_parent_cur.data(), public_parent_cur_cStr, strLen );
                    public_parent_cur[strLen] = '\n';
                    stor->setBlob( "public_parent_cur", public_parent_cur );
                }
            }
        }
    }
}
#endif  // CONFIG_LOCAL_MODE

std::list<PeerInfo> InfoManager::getPeersInfo() {
    loadPeers();
    std::list<PeerInfo> res;
    for( std::map<std::string, PeerInfo>::iterator it = m_Peers.begin(); it != m_Peers.end(); ++it ) {
        res.push_back( it->second );
    }

    return res;
}

bool InfoManager::addOrUpdatePeersIp( const std::string& uuid, std::list<std::string>& ip ) {
    loadPeers();
    const auto it = m_Peers.find( uuid );

    if( it != m_Peers.end() ) {
        // update ip
        PeerInfo& peer = m_Peers.find( uuid )->second;
        peer.setIp( ip );
        m_Peers.at( uuid ) = peer;
        peer.setId( uuid );
    }
    else {
        // add new peer with ip
        std::list<std::string> tokens;
        PeerInfo peer;
        peer.setIp( ip );
        peer.setId( uuid );
        m_Peers.insert( std::pair<std::string, PeerInfo>( uuid, peer ) );
    }

    return savePeers();
}

bool InfoManager::storePeersData( const gason::JsonValue& json ) {
    std::string out;
    std::string uuid = "";
    auto peer = json.toNode();

    while( peer ) {
        PeerInfo peerInfo;
        std::string uuid = peer->key;
        // Store tokens
        std::list<std::string> tokens;

        if( peer->value.isNode() ) {
            auto param = peer->value.toNode();
            while( param ) {
                if( param->value.isString() ) {
                    if( !strcmp( param->key, "id" ) ) {
                        peerInfo.setId( param->value.toString() );
                    }
                    else if( !strcmp( param->key, SERIAL_KEY ) ) {
                        peerInfo.setSerial( param->value.toString() );
                    }
                    else if( !strcmp( param->key, STATUS_KEY ) ) {
                        peerInfo.setStatus( param->value.toString() );
                    }

                    param = param->next;

                    continue;
                }

                if( param->value.isArray() ) {
                    int i = 0;
                    auto arr = param->value;
                    while( arr[i] ) {
                        if( param->value[i].isString() ) {
                            tokens.push_back( param->value[i].toString() );
                        }

                        i++;
                    }
                }

                peerInfo.setTokens( tokens );
                m_Peers.insert( std::pair<std::string, PeerInfo>( uuid, peerInfo ) );
                param = param->next;
            }
        }
        peer = peer->next;
    }

    return savePeers();
}

std::list<std::string> InfoManager::getTokens() {
    std::list<std::string> res;
    if( loadPeers() ) {
        for( std::map<std::string, PeerInfo>::iterator it = m_Peers.begin(); it != m_Peers.end(); ++it ) {
            auto tokens = it->second.tokens();
            for( const auto token : tokens ) {
                res.push_back( token );
            }
        }
    }

    return res;
}

InfoManager::ClbkRet InfoManager::updateComponent( UpdContext& ctx ) {
    if( !get_update() ) {
        return ClbkRet::Continue;
    }

    sync_local_mode();

    return ClbkRet::Continue;
}

InfoManager::StorUPtr InfoManager::getStor() const {
    return storage::BinaryFileStorageFactory::get().openNamespace( storage::section::LOCAL_MODE_CLIENT );
}

void InfoManager::visit( const bus::BusAddr from,
                         const bus::BusAddr to,
                         const cloud_client::responces::ConnectedToCloud& command ) {
    set_update( true );
}

}  // namespace peers
}  // namespace provision
