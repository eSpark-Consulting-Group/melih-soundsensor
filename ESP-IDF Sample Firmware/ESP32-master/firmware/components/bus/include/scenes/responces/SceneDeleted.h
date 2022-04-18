#pragma once

#include "BusCommand.h"

#include <string>

namespace scenes {
namespace responces {

class SceneDeleted : public bus::IBusCommand {
    std::string m_sceneId;

public:
    SceneDeleted( const std::string& sceneId )
    : m_sceneId( sceneId ) {}

    SceneDeleted( std::string&& sceneId )
    : m_sceneId( std::move( sceneId ) ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    const auto& sceneId() const {
        return m_sceneId;
    }
};

}  // namespace responces
}  // namespace scenes
