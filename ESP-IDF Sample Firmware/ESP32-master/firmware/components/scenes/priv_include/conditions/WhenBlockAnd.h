#pragma once

#include "WhenBlock.h"

namespace scenes {

#ifdef CONFIG_ADVANCED_SCENES
class WhenBlockAnd : public WhenBlock {
public:
    using ConditionsType = UuidSet;

    WhenBlockAnd() = default;

    WhenBlockAnd( const Uuid& uuid, const ParentType parentType, const Uuid& parentUuid, ConditionsType&& conditions )
    : WhenBlock( uuid, parentType, parentUuid )
    , m_conditions( std::move( conditions ) ) {}

    inline Type type() const override {
        return Type::WhenBlockAnd;
    }

    inline void accept( WhenBlockConstVisitor& visitor ) const override {
        visitor.visit( *this );
    }

    inline const auto& conditions() const {
        return m_conditions;
    }

private:
    bool writeImpl( common::IWriteStream& stream ) const override {
        return stream.write( m_conditions );
    }

    bool readImpl( common::IReadStream& stream ) override {
        return stream.read( m_conditions );
    }

private:
    ConditionsType m_conditions;
};
#endif  // CONFIG_ADVANCED_SCENES

}  // end of namespace scenes
