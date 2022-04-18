#pragma once

#include "WhenBlock.h"

namespace scenes {

class WhenBlockEqual final : public WhenBlock {
public:
    using ConditionsType = UuidSet;

    WhenBlockEqual() = default;

    WhenBlockEqual( const Uuid& uuid, const ParentType parentType, const Uuid& parentUuid, ConditionsType&& conditions )
    : WhenBlock( uuid, parentType, parentUuid )
    , m_conditions( std::move( conditions ) ) {}

    inline Type type() const override {
        return Type::WhenBlockEqual;
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

}  // end of namespace scenes
