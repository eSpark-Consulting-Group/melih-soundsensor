#pragma once

#include "WhenBlock.h"

namespace scenes {

class WhenBlockHasAtLeastOneDictionaryValue : public WhenBlock {
public:
    WhenBlockHasAtLeastOneDictionaryValue() = default;

    WhenBlockHasAtLeastOneDictionaryValue( const Uuid& uuid,
                                           const ParentType parentType,
                                           const Uuid& parentUuid,
                                           StringType&& itemId,
                                           ValueTypeUPtr&& value )
    : WhenBlock( uuid, parentType, parentUuid )
    , m_itemId( std::move( itemId ) )
    , m_value( std::move( value ) ) {}

    inline Type type() const override {
        return Type::WhenBlockHasAtLeastOneDictionaryValue;
    }

    inline void accept( WhenBlockConstVisitor& visitor ) const override {
        visitor.visit( *this );
    }

    inline const auto& itemId() const {
        return m_itemId;
    }

    inline const auto& value() const {
        return m_value;
    }

private:
    bool writeImpl( common::IWriteStream& stream ) const override {
        return stream.write( m_itemId ) && stream.write( m_value );
    }

    bool readImpl( common::IReadStream& stream ) override {
        return stream.read( m_itemId ) && stream.read( m_value );
    }

private:
    StringType m_itemId;
    ValueTypeUPtr m_value;
};

}  // end of namespace scenes
