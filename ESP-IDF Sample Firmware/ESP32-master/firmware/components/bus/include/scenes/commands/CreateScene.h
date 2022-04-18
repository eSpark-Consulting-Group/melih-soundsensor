#pragma once

#include "Base.h"
#include "gason.hpp"

namespace scenes {
namespace commands {

class CreateScene : public Base {
public:
    CreateScene( const gason::JsonValue& params )
    : m_params( params ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline const auto& params() const {
        return m_params;
    }

    inline auto& params() {
        return m_params;
    }

private:
    gason::JsonValue m_params;
};

}  // namespace commands
}  // namespace scenes
