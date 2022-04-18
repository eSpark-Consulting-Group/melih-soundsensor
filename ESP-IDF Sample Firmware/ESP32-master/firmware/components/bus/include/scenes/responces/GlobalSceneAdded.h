#pragma once

#include "SceneAdded.h"

namespace scenes {
namespace responces {

class GlobalSceneAdded final : public SceneAdded {
public:
    using SceneAdded::SceneAdded;

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

}  // namespace responces
}  // namespace scenes
