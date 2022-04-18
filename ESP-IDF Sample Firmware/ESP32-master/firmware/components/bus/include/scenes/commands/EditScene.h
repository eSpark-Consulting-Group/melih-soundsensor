#pragma once

#include "CreateScene.h"

namespace scenes {
namespace commands {

class EditScene final : public CreateScene {
public:
    EditScene( std::string&& sceneId, const gason::JsonValue& params )
    : CreateScene( params )
    , m_sceneId( std::move( sceneId ) ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline const auto& sceneId() const {
        return m_sceneId;
    }

private:
    std::string m_sceneId;
};

}  // namespace commands
}  // namespace scenes
