#pragma once

#include "WhenBlock.h"

namespace scenes {

#ifdef CONFIG_ADVANCED_SCENES
class WhenBlockNot : public WhenBlock {
public:
    WhenBlockNot() = default;

    WhenBlockNot( const Uuid& uuid, const ParentType parentType, const Uuid& parentUuid, Uuid&& condition )
    : WhenBlock( uuid, parentType, parentUuid )
    , m_condition( std::move( condition ) ) {}

    inline Type type() const override {
        return Type::WhenBlockNot;
    }

    inline void accept( WhenBlockConstVisitor& visitor ) const override {
        visitor.visit( *this );
    }

    inline const auto& condition() const {
        return m_condition;
    }

private:
    bool writeImpl( common::IWriteStream& stream ) const override {
        return stream.write( m_condition );
    }

    bool readImpl( common::IReadStream& stream ) override {
        return stream.read( m_condition );
    }

private:
    Uuid m_condition;
};
#endif  // CONFIG_ADVANCED_SCENES

}  // end of namespace scenes
