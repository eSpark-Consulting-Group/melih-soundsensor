#pragma once

#include "Common.h"
#include "WhenBlock.h"
#include "ThenBlock.h"

#include <list>
#include <string>
#include <memory>

namespace scenes {

template <typename SceneType>
class Scene {
public:
    using UPtr = std::unique_ptr<SceneType>;
    using HouseModesList = std::list<StringType>;
    using UserNotificationsList = std::list<StringType>;

    enum class ExecPolicy : std::uint8_t { CheckActionResult, IgnoreActionResult };

    static const auto SaveBufferSize = BufferStreamType::MiddleBufSize;

    constexpr static const char* StorPrefix = "scene_";
    constexpr static const char* PersistentStorPrefix = "prsnt_";

    static UPtr create( KVSValue& value );

    static UPtr create( const BlobType& blob );

    static UPtr create( common::IReadStream& stream );

    static UPtr create( const StringType& storageId, storage::KeyValueStorage& stor );

    static UPtr create( const Uuid uuid, storage::KeyValueStorage& stor ) {
        return create( uuid.toHexString( StorPrefix ), stor );
    }

    Scene() = default;

    Scene( Uuid uuid,
           const char* name,
           UuidSet&& whenBlocks,
           UuidList&& thenBlocks,
           bool isEnabled,
           ExecPolicy execPolicy );

    Scene( Uuid uuid,
           StringType&& name,
           UuidSet&& whenBlocks,
           UuidList&& thenBlocks,
           bool isEnabled,
           ExecPolicy execPolicy );

    virtual ~Scene() = default;

    const Uuid& uuid() const {
        return m_uuid;
    }

    static StringType storKey( const Uuid& sceneUuid );

    inline auto storageId() const {
        return m_uuid.toHexString( StorPrefix );
    }

    inline const auto& name() const {
        return m_name;
    }

    inline bool isValidName() const {
        return !name().empty();
    }

    inline void setName( StringType&& newName ) {
        m_name = std::move( newName );
    }

    inline const auto& conditions() const {
        return m_whenBlocks;
    }

    inline auto& conditions() {
        return m_whenBlocks;
    }

    inline const auto& actions() const {
        return m_thenBlocks;
    }

    inline auto& actions() {
        return m_thenBlocks;
    }

    inline bool isEnabled() const {
        return m_isEnabled;
    }

    inline void setEnabled( bool isEnabled ) {
        m_isEnabled = isEnabled;
    }

    inline const auto& persistentStorId() const {
        return m_persistentStorId;
    }

    inline void setPersistentStorId( StringType&& persistentId ) {
        m_persistentStorId = std::move( persistentId );
    }

    inline void setPersistentStorId( const StringType& persistentId ) {
        if( persistentId != storageId() ) {
            m_persistentStorId = persistentId;
        }
    }

    auto externStorId() const {
        return persistentStorId().empty() ? storageId() : persistentStorId();
    }

    virtual bool read( common::IReadStream& stream ) = 0;

    virtual bool write( common::IWriteStream& stream ) const = 0;

    bool save( storage::KeyValueStorage& stor ) const;

    bool remove( const StringType& scenesPath, storage::KeyValueStorage& stor ) const;

    inline auto execPolicy() const {
        return m_execPolicy;
    }

    UPtr clone() const {
        auto newScene = std::make_unique<SceneType>();
        *newScene = *static_cast<const SceneType*>( this );
        return std::move( newScene );
    }

protected:
    Uuid m_uuid;
    StringType m_persistentStorId;
    StringType m_name;
    UuidSet m_whenBlocks;
    UuidList m_thenBlocks;
    bool m_isEnabled;
    ExecPolicy m_execPolicy;
};

}  // end of namespace scenes

#include "../src/SceneImpl.hpp"
