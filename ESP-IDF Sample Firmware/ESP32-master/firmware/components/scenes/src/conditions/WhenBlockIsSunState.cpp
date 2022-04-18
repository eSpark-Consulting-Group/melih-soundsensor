#include "conditions/WhenBlockIsSunState.h"

namespace scenes {

TimeOfDay WhenBlockIsSunState::trigTime( const TimeInfo& timeInfo ) const {
    return ( SunMode::Sunrise == sunMode() ? getSunriseOfDay( timeInfo ) : getSunsetOfDay( timeInfo ) ) +
           m_timeInterval;
}

bool WhenBlockIsSunState::writeImpl( common::IWriteStream& stream ) const {
    return WhenBlockIsDateBase::writeImpl( stream ) && stream.write( m_sunMode ) && stream.write( m_timeInterval );
}

bool WhenBlockIsSunState::readImpl( common::IReadStream& stream ) {
    return WhenBlockIsDateBase::readImpl( stream ) && stream.read( m_sunMode ) && stream.read( m_timeInterval );
}

}  // end of namespace scenes
