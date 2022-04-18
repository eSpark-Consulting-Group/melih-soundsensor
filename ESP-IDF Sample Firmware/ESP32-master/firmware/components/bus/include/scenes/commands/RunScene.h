#pragma once

#include "DeleteScene.h"

namespace scenes {
namespace commands {

class RunScene : public DeleteScene {
public:
    using DeleteScene::DeleteScene;

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

}  // namespace commands
}  // namespace scenes
