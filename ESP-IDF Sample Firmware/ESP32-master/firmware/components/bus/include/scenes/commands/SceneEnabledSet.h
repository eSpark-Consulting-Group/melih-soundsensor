#pragma once

#include "Base.h"

namespace scenes {
namespace commands {

class SceneEnabledSet : public Base {
public:
    SceneEnabledSet( std::string&& sceneId, const bool isEnabled )
    : m_sceneId( std::move( sceneId ) )
    , m_isEnabled( isEnabled ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline const auto& sceneId() const {
        return m_sceneId;
    }

    inline auto isEnabled() const {
        return m_isEnabled;
    }

private:
    std::string m_sceneId;
    bool m_isEnabled;
};

}  // namespace commands
}  // namespace scenes
