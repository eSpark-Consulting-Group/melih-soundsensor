#include "conditions/WhenBlockIsDateBase.h"

namespace scenes {

bool WhenBlockIsDateBase::isDayOfWeek( const TimePointType& tp ) const {
    const auto& wdays = weekdays();
    if( wdays.empty() ) {
        return true;
    }

    auto currWDay = getDayOfWeek( toTimeInfo( tp ) );
    if( 0 == currWDay ) {
        currWDay = 7;
    }
    for( const auto wd : wdays ) {
        if( wd == currWDay ) {
            return true;
        }
        else if( wd > currWDay ) {
            return false;
        }
    }

    return false;
}

bool WhenBlockIsDateBase::isDayOfMonth( const TimePointType& tp ) const {
    const auto& mdays = days();
    if( mdays.empty() ) {
        return true;
    }

    const auto currMDay = getDayOfMonth( toTimeInfo( tp ) );
    for( const auto md : mdays ) {
        if( md == currMDay ) {
            return true;
        }
        else if( md > currMDay ) {
            return false;
        }
    }

    return false;
}

bool WhenBlockIsDateBase::writeImpl( common::IWriteStream& stream ) const {
    return stream.write( m_weekdays ) && stream.write( m_days );
}

bool WhenBlockIsDateBase::readImpl( common::IReadStream& stream ) {
    return stream.read( m_weekdays ) && stream.read( m_days );
}

}  // end of namespace scenes
