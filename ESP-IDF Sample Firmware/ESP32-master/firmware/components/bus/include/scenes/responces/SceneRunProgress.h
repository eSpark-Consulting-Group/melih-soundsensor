#pragma once

#include "BusCommand.h"

#include <list>
#include <string>

namespace scenes {
namespace responces {

class SceneRunProgress : public bus::IBusCommand {
public:
    using StringType = std::string;

    SceneRunProgress( const StringType& sceneId,
                      const StringType& status,
                      const StringType& sceneName,
                      const bool isUserNotifications,
                      const std::list<StringType>& notificationsList,
                      const StringType& roomId )
    : m_sceneId( sceneId )
    , m_status( status )
    , m_sceneName( sceneName )
    , m_isUserNotifications( isUserNotifications )
    , m_notifications( notificationsList )
    , m_roomId( roomId ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline const auto& status() const {
        return m_status;
    }

    inline const auto& sceneId() const {
        return m_sceneId;
    }

    inline const auto& sceneName() const {
        return m_sceneName;
    }

    inline const auto& roomId() const {
        return m_roomId;
    }

    inline const auto& isUserNotifications() const {
        return m_isUserNotifications;
    }

    inline const auto& notificationsList() const {
        return m_notifications;
    }

private:
    StringType m_sceneId;
    StringType m_status;
    StringType m_sceneName;
    bool m_isUserNotifications;
    std::list<StringType> m_notifications;
    StringType m_roomId;
};

}  // namespace responces
}  // namespace scenes
