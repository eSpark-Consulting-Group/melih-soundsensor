#pragma once

#include "Base.h"

#include <string>

namespace scenes {
namespace commands {

class JsonifyScene : public Base {
public:
    using StringType = std::string;

    JsonifyScene( common::JsonStreamGenerator& gen, const StringType& sceneId )
    : m_gen( gen )
    , m_sceneId( sceneId ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline auto& gen() const {
        return m_gen;
    }

    inline const auto& sceneId() const {
        return m_sceneId;
    }

private:
    common::JsonStreamGenerator& m_gen;
    const StringType m_sceneId;
};

}  // namespace commands
}  // namespace scenes
