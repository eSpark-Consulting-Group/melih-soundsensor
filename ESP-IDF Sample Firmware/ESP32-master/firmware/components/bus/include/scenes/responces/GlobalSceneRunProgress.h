#pragma once

#include "BusCommand.h"

#include <list>
#include <string>

namespace scenes {
namespace responces {

class GlobalSceneRunProgress : public bus::IBusCommand {
public:
    using StringType = std::string;

    GlobalSceneRunProgress( const StringType& sceneId,
                            const StringType& status,
                            const StringType& sceneName,
                            const StringType& tag )
    : m_sceneId( sceneId )
    , m_status( status )
    , m_sceneName( sceneName )
    , m_tag( tag ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline const auto& sceneId() const {
        return m_sceneId;
    }

    inline const auto& status() const {
        return m_status;
    }

    inline const auto& sceneName() const {
        return m_sceneName;
    }

    inline const auto& tag() const {
        return m_tag;
    }

private:
    StringType m_sceneId;
    StringType m_status;
    StringType m_sceneName;
    StringType m_tag;
};

}  // namespace responces
}  // namespace scenes
