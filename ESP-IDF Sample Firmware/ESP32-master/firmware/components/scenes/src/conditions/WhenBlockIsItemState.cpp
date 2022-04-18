#include "conditions/WhenBlockIsItemState.h"

namespace scenes {

WhenBlockIsItemState::WhenBlockIsItemState( const Uuid& uuid,
                                            const ParentType parentType,
                                            const Uuid& parentUuid,
                                            const BlockType blockType,
                                            StringType&& itemId,
                                            ValueTypeUPtr&& value,
                                            const ArmedState armedState,
                                            const CompareMethod compareMethod )
: WhenBlock( uuid, parentType, parentUuid )
, m_blockType( blockType )
, m_itemId( std::move( itemId ) )
, m_value( std::move( value ) )
, m_armedState( armedState )
, m_compareMethod( compareMethod ) {}

bool WhenBlockIsItemState::writeImpl( common::IWriteStream& stream ) const {
    return stream.write( m_blockType ) && stream.write( m_itemId ) && stream.write( m_value ) &&
           stream.write( m_armedState ) && stream.write( m_compareMethod );
}

bool WhenBlockIsItemState::readImpl( common::IReadStream& stream ) {
    return stream.read( m_blockType ) && stream.read( m_itemId ) && stream.read( m_value ) &&
           stream.read( m_armedState ) && stream.read( m_compareMethod );
}

}  // end of namespace scenes
