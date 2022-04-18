#pragma once

#include "Base.h"

namespace scenes {
namespace commands {

class SetSceneName final : public Base {
public:
    SetSceneName( std::string&& sceneId, std::string&& sceneName )
    : m_sceneId( std::move( sceneId ) )
    , m_sceneName( std::move( sceneName ) ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline auto sceneId() const {
        return m_sceneId;
    }

    inline auto sceneNewName() const {
        return m_sceneName;
    }

private:
    std::string m_sceneId;
    std::string m_sceneName;
};

}  // namespace commands
}  // namespace scenes
