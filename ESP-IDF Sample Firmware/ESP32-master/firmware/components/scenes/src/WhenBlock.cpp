#include "WhenBlock.h"
#include "JsonGenerator.h"
#include "CommonResources.h"
#include "KeyValueStorage.h"
#include "dev_dis/DevDiscovery.h"

#include "conditions/Conditions.h"

#include <cstdio>
#include <stack>

#include "DebugUtils.h"

namespace scenes {

using namespace common::resources;

const char* WhenBlock::StorPrefix = "when_";

namespace {

class WhenBlockRemover : WhenBlockConstVisitor {
public:
    bool remove( const Uuid& uuid, storage::KeyValueStorage& stor ) {
        bool isSuccess = true;
        {
            auto wblock = WhenBlock::create( uuid, stor );
            if( !wblock ) {
                stor.remove( WhenBlock::storKey( uuid ) );
                return false;
            }

            wblock->accept( *this );

            if( !stor.remove( wblock->storKey() ) ) {
                isSuccess = false;
            }
        }

        while( !m_itemsToRemove.empty() ) {
            const auto childBlockUuid = std::move( m_itemsToRemove.top() );
            m_itemsToRemove.pop();
            if( !this->remove( childBlockUuid, stor ) ) {
                isSuccess = false;
            }
        }

        return isSuccess;
    }

private:
#ifdef CONFIG_ADVANCED_SCENES
    void visit( const WhenBlockAnd& block ) override {
        for( const auto& uuid : block.conditions() ) {
            m_itemsToRemove.emplace( uuid );
        }
    };

    void visit( const WhenBlockOr& block ) override {
        for( const auto& uuid : block.conditions() ) {
            m_itemsToRemove.emplace( uuid );
        }
    };

    void visit( const WhenBlockNot& block ) override {
        m_itemsToRemove.emplace( block.condition() );
    };
#endif  // CONFIG_ADVANCED_SCENES

    void visit( const WhenBlockEqual& block ) override {
        for( const auto& uuid : block.conditions() ) {
            m_itemsToRemove.emplace( uuid );
        }
    };

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
    void visit( const WhenBlockIsUnknowDeviceDetected& block ) override {
        auto instance = dev_dis::DevDiscovery::get();
        if( instance ) {
            instance->stopUnknowDevDetection();
        }
    }

    void visit( const WhenBlockIsDeviceDetected& block ) override {
        auto instance = dev_dis::DevDiscovery::get();
        if( instance ) {
            instance->stopScan();
        }
    }
#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

private:
    std::stack<Uuid> m_itemsToRemove;
};

}  // namespace

WhenBlock::UPtr WhenBlock::create( KVSValue& value ) {
    if( !value.is_type<BlobType>() ) {
        return nullptr;
    }
    auto blob = value.get<BlobType>();
    return create( blob );
}

WhenBlock::UPtr WhenBlock::create( const BlobType& blob ) {
    common::ArrayRStream stream( blob.data(), blob.size() );
    UPtr blockUPtr;
    return stream.read( blockUPtr ) ? std::move( blockUPtr ) : nullptr;
}

WhenBlock::UPtr WhenBlock::create( common::IReadStream& stream ) {
    {
        SerializationMarker marker;
        if( !stream.read( marker ) || marker != TypeMarker ) {
            FUNC_LOGE( "WhenBlock::create: Can't read when block type serialization marker!" );
            return nullptr;
        }
    }

    Type type;
    if( !stream.read( type ) ) {
        FUNC_LOGE( "WhenBlock::create: Can't read when block type!" );
        return nullptr;
    }

    UPtr blockPtr;
    switch( type ) {
    case Type::WhenBlockInterval: {
        blockPtr = std::make_unique<WhenBlockInterval>();
        break;
    }
    case Type::WhenBlockIsDate: {
        blockPtr = std::make_unique<WhenBlockIsDate>();
        break;
    }
    case Type::WhenBlockIsSunState: {
        blockPtr = std::make_unique<WhenBlockIsSunState>();
        break;
    }
    case Type::WhenBlockIsOnce: {
        UPtr blockPtr = std::make_unique<WhenBlockIsOnce>();
        return stream.read( *blockPtr ) ? std::move( blockPtr ) : nullptr;
    }
    case Type::WhenBlockIsItemState: {
        blockPtr = std::make_unique<WhenBlockIsItemState>();
        break;
    }
#ifdef CONFIG_ADVANCED_SCENES
    case Type::WhenBlockAnd: {
        blockPtr = std::make_unique<WhenBlockAnd>();
        break;
    }
    case Type::WhenBlockOr: {
        blockPtr = std::make_unique<WhenBlockOr>();
        break;
    }
    case Type::WhenBlockNot: {
        blockPtr = std::make_unique<WhenBlockNot>();
        break;
    }
#endif  // CONFIG_ADVANCED_SCENES
    case Type::WhenBlockEqual: {
        blockPtr = std::make_unique<WhenBlockEqual>();
        break;
    }
    case Type::WhenBlockIsUserLockOperation: {
        blockPtr = std::make_unique<WhenBlockIsUserLockOperation>();
        break;
    }
    case Type::WhenBlockHasAtLeastOneDictionaryValue: {
        blockPtr = std::make_unique<WhenBlockHasAtLeastOneDictionaryValue>();
        break;
    }
    case Type::WhenBlockIsUnknowDeviceDetected: {
        blockPtr = std::make_unique<WhenBlockIsUnknowDeviceDetected>();
        break;
    }
    case Type::WhenBlockIsDeviceDetected: {
        blockPtr = std::make_unique<WhenBlockIsDeviceDetected>();
        break;
    }
    case Type::WhenBlockIsDateRange: {
        blockPtr = std::make_unique<WhenBlockIsDateRange>();
        break;
    }
    default:
        break;
    }

    return blockPtr && blockPtr->read( stream ) ? std::move( blockPtr ) : nullptr;
}

WhenBlock::UPtr WhenBlock::create( const StringType& storKey, storage::KeyValueStorage& stor ) {
    BlobType blob;
    if( stor.getBlob( storKey, blob ) ) {
        return create( blob );
    }

    FUNC_LOGE( "WhenBlock::create: Can't get block blob for %s!", storKey.c_str() );
    return nullptr;
}

bool WhenBlock::remove( const Uuid& uuid, storage::KeyValueStorage& stor ) {
    WhenBlockRemover remover;
    return remover.remove( uuid, stor );
}

WhenBlock::WhenBlock( const Uuid& uuid, const ParentType parentType, const Uuid& parentUuid )
: m_uuid( uuid )
, m_parentType( parentType )
, m_parentUuid( parentUuid ) {}

StringType WhenBlock::storKey( const Uuid& blockUuid ) {
    return blockUuid.toHexString( StorPrefix );
}

StringType WhenBlock::storKey() const {
    return m_uuid.toHexString( StorPrefix );
}

bool WhenBlock::save( storage::KeyValueStorage& stor ) const {
    BufferStreamType stream( SaveBufferSize );
    return stream && stream.write( *this ) && stor.setBlob( storKey(), stream, stream.size() );
}

bool WhenBlock::write( common::IWriteStream& stream ) const {
    return stream.write( TypeMarker ) && stream.write( type() ) && stream.write( ObjectDataMarker ) &&
           stream.write( uuid() ) && stream.write( parentType() ) && stream.write( parentUuid() ) &&
           writeImpl( stream );
}

bool WhenBlock::read( common::IReadStream& stream ) {
    SerializationMarker marker;
    {
        if( !stream.read( marker ) ) {
            return false;
        }
        Type typeId;  // we should skip it there
        if( TypeMarker == marker && ( !stream.read( typeId ) || !stream.read( marker ) ) ) {
            return false;
        }

        if( ObjectDataMarker != marker ) {
            return false;
        }
    }
    Uuid blockUuid, parentUuid;
    ParentType parentType;
    if( !stream.read( blockUuid ) || !stream.read( parentType ) || !stream.read( parentUuid ) ) {
        return false;
    }

    setUuid( std::move( blockUuid ) );
    setParentType( parentType );
    setParentUuid( std::move( parentUuid ) );
    return readImpl( stream );
}

}  // end of namespace scenes
