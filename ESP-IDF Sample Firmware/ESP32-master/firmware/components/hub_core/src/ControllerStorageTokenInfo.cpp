#include "ControllerStorageTokenInfo.h"
#include "CommonResources.h"
#include <BinaryFileStorageFactory.h>
#include <StorageSection.h>
#include "IRamBufStream.h"

#include "DebugUtils.h"

namespace core {

using BufferStreamType = common::IRamBufferStream;
static constexpr const char* ControllerStorageTokenKey = "ControllerStorageTokenKey";

bool ControllerStorageToken::read( common::IReadStream& stream ) {
    auto res = stream.read( m_tokenString ) && stream.read( m_expiresTs ) && stream.read( m_xapp ) &&
               stream.read( m_uploadEndpoints ) && stream.read( m_downloadEndpoints );

    return res;
}

bool ControllerStorageToken::write( common::IWriteStream& stream ) const {
    return stream.write( m_tokenString ) && stream.write( m_expiresTs ) && stream.write( m_xapp ) &&
           stream.write( m_uploadEndpoints ) && stream.write( m_downloadEndpoints );
}

bool ControllerStorageToken::load() {
    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::CONTROLLER_STORAGE_INFO );

    if( !stor ) {
        return false;
    }

    storage::KeyValueStorage::BlobType blob;
    if( stor->getBlob( ControllerStorageTokenKey, blob ) ) {
        common::ArrayRStream stream( blob.data(), blob.size() );
        if( read( stream ) ) {
            return true;
        }
        else {
            FUNC_LOGE( "Can't load hub settings from storage!" );
            return false;
        }
    }
    return false;
}

bool ControllerStorageToken::save() {
    BufferStreamType stream( BufferStreamType::MiddleBufSize );

    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::CONTROLLER_STORAGE_INFO );

    bool is_successfull =
        stream && write( stream ) && stor->setBlob( ControllerStorageTokenKey, stream, stream.size() );
    if( is_successfull ) {
        FUNC_LOGI( "Save controller storage token to storage - done" );
    }
    else {
        FUNC_LOGE( "Save controller storage token to storage - error" );
    }

    return is_successfull;
}

}  // namespace core
