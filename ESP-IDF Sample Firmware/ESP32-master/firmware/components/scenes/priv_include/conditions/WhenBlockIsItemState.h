#pragma once

#include "WhenBlock.h"

namespace scenes {

class WhenBlockIsItemState : public WhenBlock {
public:
    enum class BlockType : std::uint8_t { ItemState, CompareNumber, IsButtonState };

    enum ArmedState { None, IsArmed, IsDisarmed };

    enum class CompareMethod { Equal, More, Less, MoreOrEqual, LessOrEqual, NotDefined };

    WhenBlockIsItemState() = default;

    WhenBlockIsItemState( const Uuid& uuid,
                          const ParentType parentType,
                          const Uuid& parentUuid,
                          const BlockType blockType,
                          StringType&& itemId,
                          ValueTypeUPtr&& value,
                          const ArmedState armedState,
                          const CompareMethod compareMethod );

    inline Type type() const override {
        return Type::WhenBlockIsItemState;
    }

    inline auto blockType() const {
        return m_blockType;
    }

    inline const auto& itemId() const {
        return m_itemId;
    }

    inline const ValueType& value() const {
        return *m_value;
    }

    inline ArmedState armedState() const {
        return m_armedState;
    }

    inline auto compareMethod() const {
        return m_compareMethod;
    }

    void accept( WhenBlockConstVisitor& visitor ) const override {
        visitor.visit( *this );
    }

private:
    bool writeImpl( common::IWriteStream& stream ) const override;

    bool readImpl( common::IReadStream& stream ) override;

private:
    BlockType m_blockType = BlockType::ItemState;
    StringType m_itemId;
    ValueTypeUPtr m_value;
    ArmedState m_armedState = None;
    CompareMethod m_compareMethod = CompareMethod::Equal;
};

}  // end of namespace scenes
