#pragma once

#include "SceneChanged.h"

namespace scenes {
namespace responces {

class GlobalSceneChanged final : public SceneChanged {
public:
    using SceneChanged::SceneChanged;

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

}  // namespace responces
}  // namespace scenes
