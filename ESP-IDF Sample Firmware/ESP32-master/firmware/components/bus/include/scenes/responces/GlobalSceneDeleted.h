#pragma once

#include "SceneDeleted.h"

namespace scenes {
namespace responces {

class GlobalSceneDeleted final : public SceneDeleted {
public:
    using SceneDeleted::SceneDeleted;

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

}  // namespace responces
}  // namespace scenes
