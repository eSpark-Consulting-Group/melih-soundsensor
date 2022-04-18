#pragma once

#include "Base.h"

#include <functional>

namespace scenes {

class LocalScene;

namespace commands {

class EnumerateLocalScenes : public Base {
public:
    using ClbkType = std::function<void( const LocalScene& )>;

    EnumerateLocalScenes( ClbkType clbk )
    : m_clbk( clbk ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline const auto& clbk() const {
        return m_clbk;
    }

private:
    ClbkType m_clbk;
};

}  // namespace commands
}  // namespace scenes
