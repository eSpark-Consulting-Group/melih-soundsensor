#pragma once

#include "IWriteStream.h"
#include "IReadStream.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <tuple>

namespace common {

class IWriteStream;
class IReadStream;

}  // end of namespace common

namespace scenes {

using ClockType = std::chrono::system_clock;
using TimeInfo = std::tm;

template <typename DurationType>
inline auto toDurationValue( const DurationType& duration ) {
    return duration.count();
}

template <typename TimePointType>
inline auto toEpochDuration( const TimePointType& pointType ) {
    return pointType.time_since_epoch();
}

class DelayTypeSec : public std::chrono::seconds {
public:
    DelayTypeSec( const std::chrono::seconds& obj )
    : std::chrono::seconds( obj ) {}

    using std::chrono::seconds::seconds;

    bool write( common::IWriteStream& stream ) const {
        auto delayValue = toDurationValue( *this );
        return stream.write( delayValue );
    }

    bool read( common::IReadStream& stream ) {
        decltype( toDurationValue( *this ) ) delayValue;
        if( !stream.read( delayValue ) ) {
            return false;
        }
        *this = decltype( *this )( delayValue );
        return true;
    }
};

template <typename DurationType>
inline DelayTypeSec toSeconds( const DurationType& duration ) {
    return std::chrono::duration_cast<std::chrono::seconds>( duration );
}

template <typename DurationType>
inline auto toMillis( const DurationType& duration ) {
    return std::chrono::duration_cast<std::chrono::milliseconds>( duration );
}

class TimePointType : public std::chrono::time_point<ClockType> {
public:
    TimePointType( const std::chrono::time_point<ClockType>& obj )
    : std::chrono::time_point<ClockType>( obj ) {}

    using std::chrono::time_point<ClockType>::time_point;

    bool write( common::IWriteStream& stream ) const {
        auto epochTimeSec = toDurationValue( toSeconds( toEpochDuration( *this ) ) );
        return stream.write( epochTimeSec );
    }

    bool read( common::IReadStream& stream ) {
        decltype( toDurationValue( toSeconds( toEpochDuration( *this ) ) ) ) epochTimeSec;
        if( !stream.read( epochTimeSec ) ) {
            return false;
        }
        DelayTypeSec durationSec( epochTimeSec );
        *this = TimePointType( durationSec );
        return true;
    }
};

inline TimePointType currentTime() {
    return ClockType::now();
}

using DurationSecValueType = decltype( toDurationValue( DelayTypeSec( 0 ) ) );

inline const TimeInfo& toTimeInfo( const TimePointType& tp ) {
    auto tt = ClockType::to_time_t( tp );
    return *std::localtime( &tt );
}

inline const auto getDayOfWeek( const TimeInfo& tInfo ) {
    return tInfo.tm_wday;
}

inline const auto getDayOfMonth( const TimeInfo& tInfo ) {
    return tInfo.tm_mday;
}

inline const auto getMonth( const TimeInfo& tInfo ) {
    return tInfo.tm_mon + 1;
}

inline const auto getYear( const TimeInfo& tInfo ) {
    return tInfo.tm_year + 1900;
}

class TimeInterval {
public:
    using ValueType = long long;
    static constexpr auto NotDefined = 0;

    static std::tuple<bool, TimeInterval> create( const char* str ) {
        int hs, mins = 0, secs = 0;
        if( nullptr == str || 1 > std::sscanf( str, "%d:%d:%d", &hs, &mins, &secs ) ) {
            return std::make_tuple( false, TimeInterval() );
        }
        return std::make_tuple( true, TimeInterval( hs, mins, secs ) );
    }

    TimeInterval() = default;

    explicit TimeInterval( const ValueType& seconds )
    : m_second( seconds ) {}

    TimeInterval( const int hour, const int minute, const int seconds = 0 )
    : m_second( 3600 * hour + 60 * minute + seconds ) {}

    TimeInterval( const TimeInfo& tInfo )
    : m_second( 3600 * tInfo.tm_hour + 60 * tInfo.tm_min + tInfo.tm_sec ) {}

    inline auto valueSeconds() const {
        return m_second;
    }

    inline void setValueSeconds( const ValueType& seconds ) {
        m_second = seconds;
    }

    std::tuple<int, int, int> hms() const {
        return std::make_tuple( m_second / 3600, ( m_second % 3600 ) / 60, m_second % 60 );
    }

    inline bool operator<( const TimeInterval& dt ) const {
        return m_second < dt.m_second;
    }

    inline bool operator>( const TimeInterval& dt ) const {
        return m_second > dt.m_second;
    }

    inline bool operator==( const TimeInterval& dt ) const {
        return m_second == dt.m_second;
    }

    inline bool operator!=( const TimeInterval& dt ) const {
        return m_second != dt.m_second;
    }

    inline bool operator>=( const TimeInterval& dt ) const {
        return m_second >= dt.m_second;
    }

    inline bool operator<=( const TimeInterval& dt ) const {
        return m_second <= dt.m_second;
    }

    TimeInterval operator-() const {
        return TimeInterval( -m_second );
    }

    inline operator bool() const {
        return NotDefined != m_second;
    }

    inline bool write( common::IWriteStream& stream ) const {
        return stream.write( m_second );
    }

    inline bool read( common::IReadStream& stream ) {
        return stream.read( m_second );
    }

    std::string toString() const {
        char buf[32];
        int hs, mins, secs;
        std::tie( hs, mins, secs ) = hms();
        snprintf( buf, sizeof( buf ), "%02i:%02i:%02i", hs, mins, secs );

        return buf;
    }

private:
    ValueType m_second = NotDefined;
};

class TimeOfDay : private TimeInterval {
public:
    using TimeInterval::hms;
    using TimeInterval::read;
    using TimeInterval::setValueSeconds;
    using TimeInterval::TimeInterval;
    using TimeInterval::toString;
    using TimeInterval::valueSeconds;
    using TimeInterval::write;

    auto hours() const {
        unsigned int hs;
        std::tie( hs, std::ignore, std::ignore ) = hms();
        return hs;
    }

    auto minutes() const {
        int ms;
        std::tie( std::ignore, ms, std::ignore ) = hms();
        return ms;
    }

    auto seconds() const {
        int ss;
        std::tie( std::ignore, std::ignore, ss ) = hms();
        return ss;
    }

    using TimeInterval::operator bool;

    inline bool operator<( const TimeOfDay& dt ) const {
        return static_cast<const TimeInterval&>( *this ) < static_cast<const TimeInterval&>( dt );
    }

    inline bool operator>( const TimeOfDay& dt ) const {
        return static_cast<const TimeInterval&>( *this ) > static_cast<const TimeInterval&>( dt );
    }

    inline bool operator==( const TimeOfDay& dt ) const {
        return static_cast<const TimeInterval&>( *this ) == static_cast<const TimeInterval&>( dt );
    }

    inline bool operator!=( const TimeOfDay& dt ) const {
        return static_cast<const TimeInterval&>( *this ) != static_cast<const TimeInterval&>( dt );
    }

    inline bool operator>=( const TimeOfDay& dt ) const {
        return static_cast<const TimeInterval&>( *this ) >= static_cast<const TimeInterval&>( dt );
    }

    inline bool operator<=( const TimeOfDay& dt ) const {
        return static_cast<const TimeInterval&>( *this ) <= static_cast<const TimeInterval&>( dt );
    }

    inline TimeOfDay& operator=( const TimeInterval& interval ) {
        setValueSeconds( interval.valueSeconds() );
        return *this;
    }

    inline TimeOfDay operator+( const TimeInterval& interval ) {
        auto secs = valueSeconds() + interval.valueSeconds();
        return TimeOfDay( secs );
    }
};

using TimeOfDaySet = std::set<TimeOfDay>;

TimeOfDay getSunriseOfDay( const TimeInfo& timeInfo );

TimeOfDay getSunsetOfDay( const TimeInfo& timeInfo );

class Month {
public:
    // https://en.wikipedia.org/wiki/Template:Month_abbreviation
    enum MonthEnum : std::uint8_t { NotDefined = 0, Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec };

    Month() = default;

    Month( const unsigned int month ) {
        if( Jan <= month && month <= Dec ) {
            m_month = MonthEnum( month );
        }
    }

    inline operator bool() const {
        return m_month != NotDefined;
    }

    inline operator unsigned int() const {
        return m_month;
    }

    inline bool write( common::IWriteStream& stream ) const {
        return stream.write( m_month );
    }

    inline bool read( common::IReadStream& stream ) {
        return stream.read( m_month );
    }

private:
    MonthEnum m_month = NotDefined;
};  // class Month

class DayOfMonth {
public:
    static constexpr auto NotDefined = 0;

    DayOfMonth() = default;

    DayOfMonth( const unsigned int day )
    : m_day( day ) {}

    inline operator bool() const {
        return m_day != NotDefined;
    }

    inline operator unsigned int() const {
        return m_day;
    }

    inline bool write( common::IWriteStream& stream ) const {
        return stream.write( m_day );
    }

    inline bool read( common::IReadStream& stream ) {
        return stream.read( m_day );
    }

private:
    std::uint8_t m_day = NotDefined;
};  // class Day

class Year {
public:
    static constexpr auto NotDefined = 0;

    Year() = default;

    Year( const unsigned int year )
    : m_year( year ) {}

    inline operator bool() const {
        return m_year != NotDefined;
    }

    inline operator unsigned int() const {
        return m_year;
    }

    inline bool write( common::IWriteStream& stream ) const {
        return stream.write( m_year );
    }

    inline bool read( common::IReadStream& stream ) {
        return stream.read( m_year );
    }

private:
    std::uint16_t m_year = NotDefined;
};

}  // end of namespace scenes
