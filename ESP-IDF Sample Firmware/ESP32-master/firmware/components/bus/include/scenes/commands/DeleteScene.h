#pragma once

#include "Base.h"

namespace scenes {
namespace commands {

class DeleteScene : public Base {
public:
    DeleteScene( std::string&& sceneId )
    : m_sceneId( std::move( sceneId ) ) {}

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
