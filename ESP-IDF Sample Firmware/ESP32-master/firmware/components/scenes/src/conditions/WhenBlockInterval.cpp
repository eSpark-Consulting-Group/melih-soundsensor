#include "conditions/WhenBlockInterval.h"

namespace scenes {

WhenBlockInterval::WhenBlockInterval( const Uuid& uuid,
                                      const ParentType parentType,
                                      const Uuid& parentUuid,
                                      const PeriodScale& periodScale,
                                      const DelayTypeSec& period,
                                      const TimePointType& creationTime )
: WhenBlock( uuid, parentType, parentUuid )
, m_scale( periodScale )
, m_period( period )
, m_creationTime( creationTime ) {}

bool WhenBlockInterval::writeImpl( common::IWriteStream& stream ) const {
    return stream.write( m_scale ) && stream.write( m_period ) && stream.write( m_creationTime );
}

bool WhenBlockInterval::readImpl( common::IReadStream& stream ) {
    return stream.read( m_scale ) && stream.read( m_period ) && stream.read( m_creationTime );
}

}  // namespace scenes
