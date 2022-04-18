#include "conditions/WhenBlockIsDate.h"

namespace scenes {

WhenBlockIsDate::DateMode WhenBlockIsDate::dateMode() const {
    if( hasDays() ) {
        return DateMode::Monthly;
    }
    else if( hasWeekdays() ) {
        return DateMode::Weekly;
    }
    else {
        return DateMode::Daily;
    }
}

bool WhenBlockIsDate::writeImpl( common::IWriteStream& stream ) const {
    return WhenBlockIsDateBase::writeImpl( stream ) && stream.write( m_hours );
}

bool WhenBlockIsDate::readImpl( common::IReadStream& stream ) {
    return WhenBlockIsDateBase::readImpl( stream ) && stream.read( m_hours );
}

}  // end of namespace scenes
