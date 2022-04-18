#include "Scene.h"

#include "CommonResources.h"
#include "KeyValueStorage.h"
#include "Storage.h"

#include <gason.hpp>

#include <algorithm>

#include "DebugUtils.h"

namespace scenes {

template <typename SceneType>
Scene<SceneType>::Scene( Uuid uuid,
                         const char* name,
                         UuidSet&& whenBlocks,
                         UuidList&& thenBlocks,
                         bool isEnabled,
                         ExecPolicy execPolicy )
: m_uuid( uuid )
, m_name( name )
, m_whenBlocks( std::move( whenBlocks ) )
, m_thenBlocks( std::move( thenBlocks ) )
, m_isEnabled( isEnabled )
, m_execPolicy( execPolicy ) {}

template <typename SceneType>
Scene<SceneType>::Scene( Uuid uuid,
                         StringType&& name,
                         UuidSet&& whenBlocks,
                         UuidList&& thenBlocks,
                         bool isEnabled,
                         ExecPolicy execPolicy )
: m_uuid( uuid )
, m_name( std::move( name ) )
, m_whenBlocks( std::move( whenBlocks ) )
, m_thenBlocks( std::move( thenBlocks ) )
, m_isEnabled( isEnabled )
, m_execPolicy( execPolicy ) {}

template <typename SceneType>
typename Scene<SceneType>::UPtr Scene<SceneType>::create( KVSValue& value ) {
    if( !value.is_type<BlobType>() ) {
        return nullptr;
    }
    auto blob = value.get<BlobType>();
    return create( blob );
}

template <typename SceneType>
typename Scene<SceneType>::UPtr Scene<SceneType>::create( const BlobType& blob ) {
    common::ArrayRStream stream( blob.data(), blob.size() );
    return create( stream );
}

template <typename SceneType>
typename Scene<SceneType>::UPtr Scene<SceneType>::create( common::IReadStream& stream ) {
    auto scene = UPtr( new SceneType );
    if( !scene || !stream.read( *scene ) ) {
        FUNC_LOGE( "Can't read Scene from stream!" );
        return nullptr;
    }

    return std::move( scene );
}

template <typename SceneType>
typename Scene<SceneType>::UPtr Scene<SceneType>::create( const StringType& storageId,
                                                          storage::KeyValueStorage& stor ) {
    BlobType blob;
    if( !stor.getBlob( storageId, blob ) ) {
        if( !stor.getBlob( PersistentStorPrefix + storageId, blob ) ) {
            FUNC_LOGE( "Can't get scene storage id for persistent scene id:%s!", storageId.c_str() );
            return nullptr;
        }
        common::ArrayRStream stream( blob.data(), blob.size() );
        StringType StorKey;
        if( !stream.read( StorKey ) || !stor.getBlob( StorKey, blob ) ) {
            FUNC_LOGE( "Can't get scene block blob for peristent scene id:%s, inner id=%s",
                       storageId.c_str(),
                       StorKey.c_str() );
            return nullptr;
        }
    }

    return create( blob );
}

template <typename SceneType>
StringType Scene<SceneType>::storKey( const Uuid& sceneUuid ) {
    return sceneUuid.toHexString( StorPrefix );
}

template <typename SceneType>
bool Scene<SceneType>::save( storage::KeyValueStorage& stor ) const {
    BufferStreamType stream( SaveBufferSize );
    if( !stream || !stream.write( *this ) || !stor.setBlob( storageId(), stream, stream.size() ) ) {
        return false;
    }

    if( persistentStorId().empty() ) {
        return true;
    }

    auto StorKey = PersistentStorPrefix + persistentStorId();
    return stream.write( storageId() ) && stor.setBlob( StorKey, stream, stream.size() );
}

template <typename SceneType>
bool Scene<SceneType>::remove( const StringType& scenesPath, storage::KeyValueStorage& stor ) const {
    bool is_success = true;
    for( const auto& uuid : m_whenBlocks ) {
        if( !WhenBlock::remove( uuid, stor ) ) {
            FUNC_LOGE( "Can't remove WhenBlock with id=%s from storage",
                       uuid.toHexString( WhenBlock::StorPrefix ).c_str() );
            is_success = false;
        }
    }
    for( const auto& uuid : m_thenBlocks ) {
        if( !ThenBlock::remove( uuid, stor ) ) {
            FUNC_LOGE( "Can't remove ThenBlock with id=%s from storage",
                       uuid.toHexString( ThenBlock::StorPrefix ).c_str() );
            is_success = false;
        }
    }

    if( !stor.remove( storageId() ) ) {
        is_success = false;
    }

    if( !storage::Storage::get().remove( scenesPath + "/" + storageId() ) ) {
        is_success = false;
    }

    if( !persistentStorId().empty() && !stor.remove( PersistentStorPrefix + persistentStorId() ) ) {
        is_success = false;
    }

    return is_success;
}

}  // end of namespace scenes
