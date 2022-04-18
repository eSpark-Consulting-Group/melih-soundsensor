#pragma once

#include "BusCommand.h"

#include <string>

namespace scenes {
namespace responces {

class SceneChanged : public bus::IBusCommand {
    using StringType = std::string;

    StringType m_sceneId;
    bool m_nameWasChanged = false;

public:
    SceneChanged( StringType&& sceneId, const bool nameWasChanged = false )
    : m_sceneId( std::move( sceneId ) )
    , m_nameWasChanged( nameWasChanged ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline const auto& sceneId() const {
        return m_sceneId;
    }

    inline const auto& nameWasChanged() const {
        return m_nameWasChanged;
    }
};

}  // namespace responces
}  // namespace scenes
