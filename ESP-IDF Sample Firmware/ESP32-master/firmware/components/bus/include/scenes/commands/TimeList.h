#pragma once

#include "ScenesList.h"

namespace scenes {
namespace commands {

class TimeList : public ScenesList {
public:
    using ScenesList::ScenesList;

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

}  // namespace commands
}  // namespace scenes
