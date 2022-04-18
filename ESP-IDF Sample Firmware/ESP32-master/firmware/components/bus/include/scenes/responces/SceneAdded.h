#pragma once

#include <string>
#include <BusCommand.h>

namespace scenes {
namespace responces {

class SceneAdded : public bus::IBusCommand {
    using StringType = std::string;

    StringType m_sceneId;

public:
    SceneAdded( StringType&& sceneId )
    : m_sceneId( std::move( sceneId ) ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline const auto& sceneId() const {
        return m_sceneId;
    }
};

}  // namespace responces
}  // namespace scenes
