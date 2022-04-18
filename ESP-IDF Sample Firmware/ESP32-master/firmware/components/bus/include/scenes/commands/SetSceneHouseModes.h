#pragma once

#include "Base.h"

#include <list>
#include <tuple>

namespace scenes {
namespace commands {

class SetSceneHouseModes final : public Base {
public:
    using HouseModesList = std::list<std::string>;

    SetSceneHouseModes( std::string&& sceneId, HouseModesList&& houseModes )
    : m_sceneId( std::move( sceneId ) )
    , m_houseModes( std::move( houseModes ) ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline auto sceneId() const {
        return m_sceneId;
    }

    inline const auto& houseModes() const {
        return m_houseModes;
    }

private:
    std::string m_sceneId;
    HouseModesList m_houseModes;
};

}  // namespace commands
}  // namespace scenes
