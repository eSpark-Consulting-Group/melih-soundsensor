#pragma once

#include "Base.h"
#include "CommonResources.h"

#include <set>
#include <string>

namespace common {
class JsonStreamGenerator;
}  // namespace common

namespace scenes {
namespace commands {

using namespace common::resources;

class ScenesData : public Base {
public:
    using StringSet = std::set<std::string>;

    ScenesData( common::JsonStreamGenerator& gen, StringSet&& scenesIds, StringSet&& fields )
    : m_jsGen( gen )
    , m_scenesIds( scenesIds )
    , m_fields( fields ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline auto& gen() const {
        return m_jsGen;
    }

    bool isScene( const std::string& sceneId ) const {
        return m_scenesIds.empty() || m_scenesIds.end() != m_scenesIds.find( sceneId );
    }

    bool isEnabledField() const {
        return m_fields.empty() || m_fields.end() != m_fields.find( tokens::Enabled );
    }

    bool isGroupIdField() const {
        return m_fields.empty() || m_fields.end() != m_fields.find( tokens::GroupId );
    }

    bool isNameField() const {
        return m_fields.empty() || m_fields.end() != m_fields.find( tokens::Name );
    }

    bool isParentIdField() const {
        return m_fields.empty() || m_fields.end() != m_fields.find( tokens::ParentId );
    }

private:
    common::JsonStreamGenerator& m_jsGen;
    StringSet m_scenesIds;
    StringSet m_fields;
};

}  // namespace commands
}  // namespace scenes
