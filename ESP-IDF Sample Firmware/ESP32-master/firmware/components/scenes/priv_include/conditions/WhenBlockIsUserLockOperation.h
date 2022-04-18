#pragma once

#include "WhenBlock.h"

namespace scenes {

class WhenBlockIsUserLockOperation : public WhenBlock {
public:
    using UserCodeType = unsigned int;
    using StateType = StringType;

    static constexpr UserCodeType InvalidUserCodeId = 0;

    WhenBlockIsUserLockOperation()
    : m_userCodeId( InvalidUserCodeId ) {}

    WhenBlockIsUserLockOperation( const Uuid& uuid,
                                  const ParentType parentType,
                                  const Uuid& parentUuid,
                                  StringType&& id,
                                  StateType&& action,
                                  const UserCodeType userCodeId )
    : WhenBlock( uuid, parentType, parentUuid )
    , m_itemId( std::move( id ) )
    , m_action( std::move( action ) )
    , m_userCodeId( userCodeId ) {}

    inline Type type() const override {
        return Type::WhenBlockIsUserLockOperation;
    }

    inline void accept( WhenBlockConstVisitor& visitor ) const override {
        visitor.visit( *this );
    }

    inline const auto& itemId() const {
        return m_itemId;
    }

    inline const auto& action() const {
        return m_action;
    }

    inline auto userCodeId() const {
        return m_userCodeId;
    }

private:
    bool writeImpl( common::IWriteStream& stream ) const override {
        return stream.write( m_itemId ) && stream.write( m_action ) && stream.write( m_userCodeId );
    }

    bool readImpl( common::IReadStream& stream ) override {
        return stream.read( m_itemId ) && stream.read( m_action ) && stream.read( m_userCodeId );
    }

private:
    StringType m_itemId;
    StateType m_action;
    UserCodeType m_userCodeId;
};

}  // end of namespace scenes
