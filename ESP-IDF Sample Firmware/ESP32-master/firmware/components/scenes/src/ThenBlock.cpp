#include "ThenBlock.h"
#include "KeyValueStorage.h"
#include "IValueVisitor.h"

#include "DebugUtils.h"

namespace scenes {

const char* ThenBlock::StorPrefix = "then_";

ThenBlock::UPtr ThenBlock::create( KVSValue& value ) {
    if( !value.is_type<BlobType>() ) {
        return nullptr;
    }
    auto blob = value.get<BlobType>();
    return create( blob );
}

ThenBlock::UPtr ThenBlock::create( const BlobType& blob ) {
    common::ArrayRStream stream( blob.data(), blob.size() );
    UPtr blockUPtr;
    return stream.read( blockUPtr ) ? std::move( blockUPtr ) : nullptr;
}

ThenBlock::UPtr ThenBlock::create( common::IReadStream& stream ) {
    {
        SerializationMarker marker;
        if( !stream.read( marker ) || marker != TypeMarker ) {
            FUNC_LOGE( "ThenBlock::create: Can't read then block type serialization marker!" );
            return nullptr;
        }
    }

    Type type;
    if( !stream.read( type ) ) {
        FUNC_LOGE( "Can't read then block type!" );
        return nullptr;
    }

    switch( type ) {
    case Type::SetItemValue: {
        UPtr blockPtr = UPtr( new ThenBlockSetItemValue() );
        return stream.read( *blockPtr ) ? std::move( blockPtr ) : nullptr;
    }
    case Type::SetDeviceArmed: {
        UPtr blockPtr = UPtr( new ThenBlockSetDeviceArmed() );
        return stream.read( *blockPtr ) ? std::move( blockPtr ) : nullptr;
    }

    case Type::SendHttpRequest: {
        UPtr blockPtr = UPtr( new ThenBlockSendHttpRequest() );
        return stream.read( *blockPtr ) ? std::move( blockPtr ) : nullptr;
    }

#ifdef CONFIG_ADVANCED_SCENES
    case Type::SwitchHouseMode: {
        UPtr blockPtr = UPtr( new ThenBlockSwitchHouseMode() );
        return stream.read( *blockPtr ) ? std::move( blockPtr ) : nullptr;
    }
    case Type::ExecuteScene: {
        UPtr blockPtr = UPtr( new ThenBlockExecuteScene() );
        return stream.read( *blockPtr ) ? std::move( blockPtr ) : nullptr;
    }
#endif  // CONFIG_ADVANCED_SCENES
    case Type::CloudCommand: {
        UPtr blockPtr = UPtr( new ThenBlockCloudCommand() );
        return stream.read( *blockPtr ) ? std::move( blockPtr ) : nullptr;
    }
    case Type::SetValueMulticast: {
        UPtr blockPtr = UPtr( new ThenBlockSetValueMulticast() );
        return stream.read( *blockPtr ) ? std::move( blockPtr ) : nullptr;
    }
    case Type::SetValueMulticast2: {
        UPtr blockPtr = UPtr( new ThenBlockSetValueMulticast2() );
        return stream.read( *blockPtr ) ? std::move( blockPtr ) : nullptr;
    }
    case Type::SetValueMulticast3: {
        UPtr blockPtr = UPtr( new ThenBlockSetValueMulticast3() );
        return stream.read( *blockPtr ) ? std::move( blockPtr ) : nullptr;
    }
    default:
        FUNC_LOGE( "Unsupported Then block type: %d", static_cast<int>( type ) );
        return nullptr;
    }
    return nullptr;
}

ThenBlock::UPtr ThenBlock::create( const Uuid uuid, storage::KeyValueStorage& stor ) {
    BlobType blob;
    if( !stor.getBlob( uuid.toHexString( StorPrefix ), blob ) ) {
        FUNC_LOGE( "Can't get then block blob for %s!", uuid.toHexString( StorPrefix ).c_str() );
        return nullptr;
    }
    return create( blob );
}

ThenBlock::ThenBlock()
: m_delay( 0 )
, m_creationTime( currentTime() ) {}

ThenBlock::ThenBlock( const Uuid& uuid, const Uuid& sceneUuid, const DelayTypeSec& delay )
: m_uuid( uuid )
, m_sceneUuid( sceneUuid )
, m_delay( delay )
, m_creationTime( currentTime() ) {}

StringType ThenBlock::storKey( const Uuid& uuid ) {
    return uuid.toHexString( StorPrefix );
}

StringType ThenBlock::storKey() const {
    return m_uuid.toHexString( StorPrefix );
}

bool ThenBlock::save( storage::KeyValueStorage& stor ) const {
    BufferStreamType stream( SaveBufferSize );
    return stream && stream.write( *this ) && stor.setBlob( storKey(), stream, stream.size() );
}

bool ThenBlock::read( common::IReadStream& stream ) {
    SerializationMarker marker;
    {
        if( !stream.read( marker ) ) {
            return false;
        }
        Type dummy;
        if( TypeMarker == marker && ( !stream.read( dummy ) || !stream.read( marker ) ) ) {
            return false;
        }
    }
    if( ObjectDataMarker != marker ) {
        return false;
    }
    return stream.read( m_uuid ) && stream.read( m_sceneUuid ) && stream.read( m_delay );
}

bool ThenBlock::write( common::IWriteStream& stream ) const {
    return stream.write( TypeMarker ) && stream.write( type() ) && stream.write( ObjectDataMarker ) &&
           stream.write( m_uuid ) && stream.write( m_sceneUuid ) && stream.write( m_delay );
}

ThenBlockSetItemValue::ThenBlockSetItemValue( const Uuid& uuid,
                                              const Uuid& sceneUuid,
                                              const DelayTypeSec& delay,
                                              const char* itemId,
                                              ValueTypeUPtr&& itemValue )
: ThenBlock( uuid, sceneUuid, delay )
, m_itemId( itemId )
, m_itemValue( std::move( itemValue ) ) {}

ThenBlockSetItemValue::ThenBlockSetItemValue( const Uuid& uuid,
                                              const Uuid& sceneUuid,
                                              const DelayTypeSec& delay,
                                              StringType&& itemId,
                                              ValueTypeUPtr&& itemValue )
: ThenBlock( uuid, sceneUuid, delay )
, m_itemId( std::move( itemId ) )
, m_itemValue( std::move( itemValue ) ) {}

bool ThenBlockSetItemValue::read( common::IReadStream& stream ) {
    return ThenBlock::read( stream ) && stream.read( m_itemId ) && stream.read( m_itemValue );
}

bool ThenBlockSetItemValue::write( common::IWriteStream& stream ) const {
    return ThenBlock::write( stream ) && stream.write( m_itemId ) && stream.write( m_itemValue );
}

int ThenBlockSetValueMulticast::itemsCount() const {
    int count = 0;
    for( const auto& item : m_valToItems ) {
        count += item.second.size();
    }
    return count;
}

bool ThenBlockSetValueMulticast::read( common::IReadStream& stream ) {
    return ThenBlock::read( stream ) && stream.read( m_valToItems );
}

bool ThenBlockSetValueMulticast::write( common::IWriteStream& stream ) const {
    return ThenBlock::write( stream ) && stream.write( m_valToItems );
}

int ThenBlockSetValueMulticast2::itemsCount() const {
    int count = 0;
    for( const auto& item : m_valToItems ) {
        count += item.second.size();
    }
    return count;
}

bool ThenBlockSetValueMulticast2::read( common::IReadStream& stream ) {
    return ThenBlock::read( stream ) && stream.read( m_valToItems );
}

bool ThenBlockSetValueMulticast2::write( common::IWriteStream& stream ) const {
    return ThenBlock::write( stream ) && stream.write( m_valToItems );
}

bool ThenBlockSetValueMulticast3::read( common::IReadStream& stream ) {
    return ThenBlock::read( stream ) && stream.read( m_valToItems ) && stream.read( m_indexes );
}

bool ThenBlockSetValueMulticast3::write( common::IWriteStream& stream ) const {
    return ThenBlock::write( stream ) && stream.write( m_valToItems ) && stream.write( m_indexes );
}

int ThenBlockSetValueMulticast3::itemsCount() const {
    int count = 0;
    for( const auto& item : m_valToItems ) {
        count += item.second.size();
    }
    return count;
}

bool ThenBlockSetValueMulticast3::addToIndex( KeyType key ) {
    if( std::find( m_indexes.begin(), m_indexes.end(), key ) == std::end( m_indexes ) ) {
        m_indexes.emplace_back( std::move( key ) );
        return true;
    }
    return false;
}

ThenBlockSetDeviceArmed::ThenBlockSetDeviceArmed( const Uuid& uuid,
                                                  const Uuid& sceneUuid,
                                                  const DelayTypeSec& delay,
                                                  StringType&& deviceId,
                                                  const bool isArmed )
: ThenBlock( uuid, sceneUuid, delay )
, m_deviceId( std::move( deviceId ) )
, m_isArmed( isArmed ) {}

bool ThenBlockSetDeviceArmed::read( common::IReadStream& stream ) {
    return ThenBlock::read( stream ) && stream.read( m_deviceId ) && stream.read( m_isArmed );
}

bool ThenBlockSetDeviceArmed::write( common::IWriteStream& stream ) const {
    return ThenBlock::write( stream ) && stream.write( m_deviceId ) && stream.write( m_isArmed );
}

ThenBlockSendHttpRequest::ThenBlockSendHttpRequest( const Uuid& uuid,
                                                    const Uuid& sceneUuid,
                                                    const DelayTypeSec& delay,
                                                    StringType&& url,
                                                    StringType&& request,
                                                    StringType&& user,
                                                    StringType&& password,
                                                    StringType&& contentType,
                                                    StringType&& content,
                                                    std::map<StringType, StringType> headers,
                                                    bool skipSecurity )
: ThenBlock( uuid, sceneUuid, delay )
, m_url( url )
, m_request( request )
, m_user( user )
, m_password( password )
, m_contentType( contentType )
, m_content( content )
, m_headers( headers )
, m_skipSecurity( skipSecurity ) {}

bool ThenBlockSendHttpRequest::read( common::IReadStream& stream ) {
    return ThenBlock::read( stream ) && stream.read( m_url ) && stream.read( m_request ) && stream.read( m_user ) &&
           stream.read( m_password ) && stream.read( m_contentType ) && stream.read( m_content ) &&
           stream.read( m_headers ) && stream.read( m_skipSecurity );
}

bool ThenBlockSendHttpRequest::write( common::IWriteStream& stream ) const {
    return ThenBlock::write( stream ) && stream.write( m_url ) && stream.write( m_request ) && stream.write( m_user ) &&
           stream.write( m_password ) && stream.write( m_contentType ) && stream.write( m_content ) &&
           stream.write( m_headers ) && stream.write( m_skipSecurity );
}

#ifdef CONFIG_ADVANCED_SCENES
ThenBlockSwitchHouseMode::ThenBlockSwitchHouseMode( const Uuid& uuid,
                                                    const Uuid& sceneUuid,
                                                    const DelayTypeSec& delay,
                                                    StringType&& houseMode )
: ThenBlock( uuid, sceneUuid, delay )
, m_houseMode( std::move( houseMode ) ) {}

bool ThenBlockSwitchHouseMode::read( common::IReadStream& stream ) {
    return ThenBlock::read( stream ) && stream.read( m_houseMode );
}

bool ThenBlockSwitchHouseMode::write( common::IWriteStream& stream ) const {
    return ThenBlock::write( stream ) && stream.write( m_houseMode );
}

ThenBlockExecuteScene::ThenBlockExecuteScene( const Uuid& uuid,
                                              const Uuid& sceneUuid,
                                              const DelayTypeSec& delay,
                                              StringType&& sceneStorId )
: ThenBlock( uuid, sceneUuid, delay )
, m_sceneStorId( std::move( sceneStorId ) ) {}

bool ThenBlockExecuteScene::read( common::IReadStream& stream ) {
    return ThenBlock::read( stream ) && stream.read( m_sceneStorId );
}

bool ThenBlockExecuteScene::write( common::IWriteStream& stream ) const {
    return ThenBlock::write( stream ) && stream.write( m_sceneStorId );
}
#endif  // CONFIG_ADVANCED_SCENES

ThenBlockCloudCommand::ThenBlockCloudCommand( const Uuid& _uuid,
                                              const Uuid& _sceneUuid,
                                              const DelayTypeSec& _delay,
                                              StringType&& _commandId,
                                              StringType&& _capability,
                                              StringType&& _command,
                                              ValueTypeUPtr&& _value )
: ThenBlock( _uuid, _sceneUuid, _delay )
, m_commandId( std::move( _commandId ) )
, m_capability( std::move( _capability ) )
, m_command( std::move( _command ) )
, m_value( std::move( _value ) ) {}

bool ThenBlockCloudCommand::read( common::IReadStream& stream ) {
    return ThenBlock::read( stream ) && stream.read( m_commandId ) && stream.read( m_capability ) &&
           stream.read( m_command ) && stream.read( m_value );
}

bool ThenBlockCloudCommand::write( common::IWriteStream& stream ) const {
    return ThenBlock::write( stream ) && stream.write( m_commandId ) && stream.write( m_capability ) &&
           stream.write( m_command ) && stream.write( m_value );
}

}  // end of namespace scenes
