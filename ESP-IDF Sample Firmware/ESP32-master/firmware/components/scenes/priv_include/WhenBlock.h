#pragma once

#include "Common.h"
#include "Time.h"
#include "IValue.h"
#include "values/UserCodeValue.h"
#include "WhenBlockVisitor.h"

#include <memory>
#include <set>
#include <cstdint>

namespace storage {
class KeyValueStorage;
}

namespace common {
class JsonStreamGenerator;
}

namespace scenes {

class WhenBlockConstVisitor;
class WhenBlockVisitor;

class WhenBlock {
    friend class common::IWriteStream;
    friend class common::IReadStream;

public:
    enum SerializationMarker { TypeMarker, ObjectDataMarker };

    enum class Type : std::uint8_t {
        WhenBlockInterval,
        WhenBlockIsDate,
        WhenBlockIsSunState,
        WhenBlockIsOnce,
        WhenBlockIsItemState,
        WhenBlockAnd,
        WhenBlockOr,
        WhenBlockEqual,
        WhenBlockNot,
        WhenBlockIsUserLockOperation,
        WhenBlockHasAtLeastOneDictionaryValue,
        WhenBlockIsUnknowDeviceDetected,
        WhenBlockIsDeviceDetected,
        WhenBlockIsDateRange
    };

    enum class ParentType : std::uint8_t { Undefined, Scene, Condition };

    using UPtr = std::unique_ptr<WhenBlock>;

    static const char* StorPrefix;

    static const auto SaveBufferSize = BufferStreamType::MiddleBufSize;

    static UPtr create( KVSValue& value );

    static UPtr create( const BlobType& blob );

    static UPtr create( common::IReadStream& stream );

    static UPtr create( const StringType& storKey, storage::KeyValueStorage& stor );

    inline static UPtr create( const Uuid& uuid, storage::KeyValueStorage& stor ) {
        return create( uuid.toHexString( StorPrefix ), stor );
    }

    static bool remove( const Uuid& uuid, storage::KeyValueStorage& stor );

    virtual ~WhenBlock() = default;

    virtual Type type() const = 0;

    inline const Uuid& uuid() const {
        return m_uuid;
    }

    inline void setUuid( Uuid&& uuid ) {
        m_uuid = std::move( uuid );
    }

    inline const auto& parentType() const {
        return m_parentType;
    }

    inline void setParentType( const ParentType parentType ) {
        m_parentType = parentType;
    }

    inline const Uuid& parentUuid() const {
        return m_parentUuid;
    }

    inline void setParentUuid( Uuid&& uuid ) {
        m_parentUuid = std::move( uuid );
    }

    virtual void accept( WhenBlockConstVisitor& visitor ) const = 0;

    static StringType storKey( const Uuid& blockUuid );

    StringType storKey() const;

    bool save( storage::KeyValueStorage& stor ) const;

    bool write( common::IWriteStream& stream ) const;

    bool read( common::IReadStream& stream );

protected:
    WhenBlock() = default;

    WhenBlock( const Uuid& uuid, const ParentType parentType, const Uuid& parentUuid );

private:
    virtual bool writeImpl( common::IWriteStream& stream ) const = 0;

    virtual bool readImpl( common::IReadStream& stream ) {
        return false;
    };

protected:
    Uuid m_uuid;
    ParentType m_parentType = ParentType::Undefined;
    Uuid m_parentUuid;
};

using ConditionsMap = std::map<Uuid, WhenBlock::UPtr>;

}  // end of namespace scenes
