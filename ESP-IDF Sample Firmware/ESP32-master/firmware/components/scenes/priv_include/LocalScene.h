#pragma once

#include "Scene.h"

namespace scenes {

class LocalScene : public Scene<LocalScene> {
public:
    LocalScene() = default;

    LocalScene( Uuid uuid,
                StringType&& name,
                const char* roomId,
                const char* groupId,
                UuidSet&& whenBlocks,
                UuidList&& thenBlocks,
                HouseModesList&& houseModes,
                UserNotificationsList&& usersToNotify,
                bool isEnabled,
                ExecPolicy execPolicy );

    LocalScene( Uuid uuid,
                const char* name,
                const char* roomId,
                const char* groupId,
                UuidSet&& whenBlocks,
                UuidList&& thenBlocks,
                HouseModesList&& houseModes,
                UserNotificationsList&& usersToNotify,
                bool isEnabled,
                ExecPolicy execPolicy );

    inline const auto& roomId() const {
        return m_roomId;
    }

    inline const auto& groupId() const {
        return m_groupId;
    }

    inline const auto& usersToNotify() const {
        return m_usersToNotify;
    }

    inline const auto& houseModes() const {
        return m_houseModes;
    }

    inline void setHouseModes( HouseModesList&& hmList ) {
        m_houseModes = std::move( hmList );
    }

    inline void setHouseModes( const HouseModesList& hmList ) {
        m_houseModes = hmList;
    }

    bool checkHouseMode( const std::uint8_t modeId ) const;

    bool checkHouseMode( const StringType& mode ) const;

    bool read( common::IReadStream& stream ) override;

    bool write( common::IWriteStream& stream ) const override;

private:
    StringType m_roomId;
    StringType m_groupId;
    HouseModesList m_houseModes;
    UserNotificationsList m_usersToNotify;
};  // class LocalScene

}  // namespace scenes
