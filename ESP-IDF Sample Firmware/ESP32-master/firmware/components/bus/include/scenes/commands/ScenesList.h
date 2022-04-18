#pragma once

#include "Base.h"

namespace common {
class JsonStreamGenerator;
}  // namespace common

namespace scenes {
namespace commands {

class ScenesList : public Base {
public:
    ScenesList( common::JsonStreamGenerator& gen, bool isOnlyId )
    : m_gen( gen )
    , m_isOnlyId( isOnlyId ) {}

    ScenesList( common::JsonStreamGenerator& gen )
    : ScenesList( gen, false ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline auto& gen() const {
        return m_gen;
    }

    inline auto isOnlyId() const {
        return m_isOnlyId;
    }

    inline void setIsOnlyId( const bool isOnlyId ) {
        m_isOnlyId = isOnlyId;
    }

private:
    common::JsonStreamGenerator& m_gen;
    bool m_isOnlyId;
};

}  // namespace commands
}  // namespace scenes
