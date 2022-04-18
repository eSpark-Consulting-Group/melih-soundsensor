#pragma once

#include <chrono>
#include <functional>
#include <vector>
#include <set>
#include <string>

namespace GregorianCalendar {
class Date;
}

namespace std {
namespace chrono {
using days = duration<int, ratio<86400> >;
using weeks = duration<int, ratio<604800> >;
using years = duration<int64_t, ratio<31536000> >;
using leap_years = duration<int64_t, ratio<31622400> >;
}  // namespace chrono
}  // namespace std

namespace TimeUtils {

struct tm stringToTimeStruct( const std::string& time, const std::string& format );
std::string timeStructToString( const tm& time, const std::string& format );
std::set<uint8_t> convertWeekdays( const std::vector<std::string>& weekDays );
GregorianCalendar::Date getDateWithModifiedTime( const GregorianCalendar::Date& date, const std::string& time );
GregorianCalendar::Date localTimeGetter();

}  // namespace TimeUtils

namespace GregorianCalendar {

using TimePoint = std::chrono::system_clock::time_point;
using ClockDuration = std::chrono::system_clock::duration;

const static double DaysInYear = 365.2421875;
const static uint8_t DaysInWeek = 7;
const static uint8_t MonthsNumberInYear = 12;
const static int64_t EpochYear = 1970;
const static int64_t EpochRegularYears = 1493;
const static int64_t EpochLeapYears = 477;
const static int64_t EpochDaysSinceAnnoDomini = 719527;
const static int8_t WeekdayInEpoch = 4;  // Thursday
const static int8_t HoursInDay = 24;
const static int8_t MinuteInHour = 60;
const static int8_t SecondsInMinute = 60;
const static int16_t MillisecondsInSecond = 1000;

class Year {
public:
    Year( const TimePoint& sinceEpoch );
    Year( int64_t year );
    Year( const Year& ) = default;
    Year( Year&& ) = default;
    Year& operator=( const Year& ) = default;
    Year& operator=( Year&& ) = default;

    void setUnit( int64_t year );
    void setUnit( const TimePoint& sinceEpoch );
    int64_t getUnit() const;
    ClockDuration toClockDuration() const;

    Year& operator++();
    Year& operator--();
    Year& operator+=( const Year& value );
    Year& operator-=( const Year& value );

    bool isLeapYear() const;
    int64_t getLeapYearsNumber();
    int64_t getRegularYearsNumber();

    static bool isLeapYear( int64_t year );
    static int64_t getLeapYearsNumber( int64_t year );
    static int64_t getUnit( const TimePoint& sinceEpoch );

private:
    std::chrono::years m_regularYears;
    std::chrono::leap_years m_leapYears;
};

class Month {
public:
    using GetCurrentDay = std::function<uint16_t()>;

public:
    Month( Year& year );

    void setUnit( const TimePoint& sinceEpoch );
    bool setUnit( uint8_t value );
    void setCurrentDayCallback( GetCurrentDay callback );
    uint8_t getUnit() const;
    uint8_t getDaysInCurrentMonth() const;
    std::chrono::days getDaysFromYearBeginning() const;
    ClockDuration toClockDuration() const;
    ClockDuration totalDuration() const;

    static uint8_t getDaysInMonth( uint8_t month );

    Month& operator++();
    Month& operator--();
    Month& operator+=( int64_t months );
    Month& operator-=( int64_t months );

private:
    Year& m_year;
    uint8_t m_month;
    GetCurrentDay m_getCurrentDay;

    const static std::vector<uint8_t> daysNumberInMonth;
};

class Day {
public:
    Day( Month& month );

    void setUnit( const TimePoint& time );
    bool setUnit( uint8_t value );
    uint8_t getUnit() const;
    ClockDuration toClockDuration() const;
    ClockDuration totalDuration() const;

    Day& operator++();
    Day& operator--();
    Day& operator+=( int64_t days );
    Day& operator-=( int64_t days );

private:
    Month& m_month;
    uint8_t m_day;
};

class Weekday {
public:
    Weekday( Day& day );

    void setUnit( const TimePoint& time );
    bool setUnit( uint8_t value );
    uint8_t getUnit() const;
    bool moveToWeekday( uint8_t value );

    Weekday& incrementWeek();
    Weekday& decrementWeek();

    Weekday& operator++();
    Weekday& operator--();
    Weekday& operator+=( int64_t days );
    Weekday& operator-=( int64_t days );

private:
    Day& m_day;
    uint8_t m_weekday;

    bool isWeekdayValid( uint8_t value );
};

template <class ChronoType, class OneLevelUpUnit, int maxLimit>
class TimeUnit {
public:
    TimeUnit( OneLevelUpUnit& levelUpUnit )
    : m_unit( 0 )
    , m_levelUpUnit( levelUpUnit ) {}

    void setUnit( const TimePoint& time ) {
        ClockDuration leftTime = time.time_since_epoch() - m_levelUpUnit.totalDuration();
        int64_t unit = leftTime / ChronoType( 1 );
        if( unit >= maxLimit ) {
            unit %= maxLimit;
        }
        m_unit = unit;
    }
    bool setUnit( uint16_t value ) {
        if( value >= maxLimit ) {
            return false;
        }
        m_unit = value;
        return true;
    }
    uint16_t getUnit() const {
        return m_unit;
    }
    ClockDuration toClockDuration() const {
        return ChronoType( m_unit );
    }
    ClockDuration totalDuration() const {
        return m_levelUpUnit.totalDuration() + toClockDuration();
    }

    TimeUnit& operator++() {
        ++m_unit;
        if( m_unit == maxLimit ) {
            m_unit = 0;
            ++m_levelUpUnit;
        }
        return *this;
    }
    TimeUnit& operator--() {
        --m_unit;
        if( m_unit == std::numeric_limits<uint16_t>::max() ) {
            m_unit = maxLimit - 1;
            --m_levelUpUnit;
        }
        return *this;
    }
    TimeUnit& operator+=( int64_t units ) {
        int64_t sum = m_unit + units;
        int64_t levelUpUnits = sum / maxLimit;
        m_unit = sum % maxLimit;
        m_levelUpUnit += levelUpUnits;
        if( sum < 0 ) {
            if( m_unit != 0 ) {
                m_unit += maxLimit;
                m_levelUpUnit -= 1;
            }
        }
        return *this;
    }
    TimeUnit& operator-=( int64_t units ) {
        return operator+=( -units );
    }

private:
    uint16_t m_unit;
    OneLevelUpUnit& m_levelUpUnit;
};

class Hour : public TimeUnit<std::chrono::hours, Day, HoursInDay> {
public:
    Hour( Day& day )
    : TimeUnit( day ) {}
};

class Minute : public TimeUnit<std::chrono::minutes, Hour, MinuteInHour> {
public:
    Minute( Hour& hour )
    : TimeUnit( hour ) {}
};

class Second : public TimeUnit<std::chrono::seconds, Minute, SecondsInMinute> {
public:
    Second( Minute& minutes )
    : TimeUnit( minutes ) {}
};

class Millisecond : public TimeUnit<std::chrono::milliseconds, Second, MillisecondsInSecond> {
public:
    Millisecond( Second& second )
    : TimeUnit( second ) {}
};

class Date {
public:
    // @brief default ctor
    Date();
    // @brief ctor sets date in human readble format. Default format is 2019-05-26T13:45:12+0200
    // @param readableTime date in human readble format
    // @param farmat definition of format. For detail infomation see decription of strftime
    // ( http://www.cplusplus.com/reference/ctime/strftime/ )
    explicit Date( const std::string& readableTime, const std::string& format = "%FT%T%z" );
    // @brief ctor sets date in unix time format
    // @param date in unix time format
    // @param timeZoneOffset time offset relative to Greenwich Mean Time
    explicit Date( const TimePoint& date, const std::chrono::seconds timeZoneOffset = std::chrono::seconds( 0 ) );
    // @brief copy ctor
    Date( const Date& date );

    // @brief copy date in unix time format
    Date& operator=( const TimePoint& date );
    // @brief copy date
    Date& operator=( const Date& date );

    // @brief set year. Using of this method might make inconsistent date, better use ctor
    void setYear( int64_t );
    // @brief set month. Using of this method might make inconsistent date, better use ctor
    bool setMonth( uint8_t );
    // @brief set day. Using of this method might make inconsistent date, better use ctor
    bool setDay( uint8_t );
    // @brief set weekday. Using of this method might make inconsistent date, better use ctor
    bool setWeekday( uint8_t );
    // @brief set hourr. Using of this method might make inconsistent date, better use ctor
    bool setHour( uint8_t );
    // @brief set minute. Using of this method might make inconsistent date, better use ctor
    bool setMinute( uint8_t );
    // @brief set second. Using of this method might make inconsistent date, better use ctor
    bool setSecond( uint8_t );
    // @brief set millisecond. Using of this method might make inconsistent date, better use ctor
    bool setMillisecond( uint16_t );
    // @brief set time zone offset. Using of this method might make inconsistent date, better use ctor
    void setTimeZoneOffset( const std::chrono::seconds& offset );

    // @brief calculates difference between current weekday and weekday is set and moves to target weekday by
    // addWeekdays method.
    bool moveToWeekday( uint8_t );

    // @brief get year
    int64_t getYear() const;
    // @brief get month
    uint8_t getMonth() const;
    // @brief get day
    uint8_t getDay() const;
    // @brief get weekday
    uint8_t getWeekday() const;
    // @brief get hour
    uint8_t getHour() const;
    // @brief get minute
    uint8_t getMinute() const;
    // @brief get second
    uint8_t getSecond() const;
    // @brief get millisecond
    uint16_t getMillisecond() const;
    // @brief get time zone offset
    std::chrono::seconds getTimeZoneOffset() const;

    // @brief move Year forward or backward
    Date& addYears( const Year& year );
    // @brief move Month forward or backward. Year might be moved too.
    Date& addMonths( int64_t );
    // @brief move Day forward or backward. Month, Year might be moved too.
    Date& addDays( int64_t );
    // @brief move Weekday forward or backward. Day, Month, Year might be moved too.
    Date& addWeekdays( int64_t );
    // @brief move Hour forward or backward. Day, Month, Year might be moved too.
    Date& addHours( int64_t );
    // @brief move Minutes forward or backward. Hour, Day, Month, Year might be moved too.
    Date& addMinutes( int64_t );
    // @brief move Second forward or backward. Minutes, Hour, Day, Month, Year might be moved too.
    Date& addSeconds( int64_t );
    // @brief move Millisecond forward or backward. Second, Minutes, Hour, Day, Month, Year might be moved too.
    Date& addMilliseconds( int64_t );

    // @brief increment Year forward or backward
    Date& incrementYear();
    // @brief increment Month forward or backward. Year might be incremented too.
    Date& incrementMonth();
    // @brief increment Day forward or backward. Month, Year might be incremented too.
    Date& incrementDay();
    // @brief increment Weekday forward or backward. Day, Month, Year might be incremented too.
    Date& incrementWeek();
    // @brief increment Hour forward or backward. Day, Month, Year might be incremented too.
    Date& incrementHour();
    // @brief increment Minutes forward or backward. Hour, Day, Month, Year might be incremented too.
    Date& incrementMinute();
    // @brief increment Second forward or backward. Minutes, Hour, Day, Month, Year might be incremented too.
    Date& incrementSecond();
    // @brief increment Millisecond forward or backward. Second, Minutes, Hour, Day, Month, Year might be
    // incremented too.
    Date& incrementMillisecond();

    // @brief decrement Year forward or backward
    Date& decrementYear();
    // @brief decrement Month forward or backward. Year might be decremented too.
    Date& decrementMonth();
    // @brief decrement Day forward or backward. Month, Year might be decremented too.
    Date& decrementDay();
    // @brief decrement Weekday forward or backward. Day, Month, Year might be decremented too.
    Date& decrementWeek();
    // @brief decrement Hour forward or backward. Day, Month, Year might be decremented too.
    Date& decrementHour();
    // @brief decrement Minutes forward or backward. Hour, Day, Month, Year might be decremented too.
    Date& decrementMinute();
    // @brief decrement Second forward or backward. Minutes, Hour, Day, Month, Year might be decremented too.
    Date& decrementSecond();
    // @brief decrement Millisecond forward or backward. Second, Minutes, Hour, Day, Month, Year might be
    // decremented too.
    Date& decrementMillisecond();

    // @brief get maximum day of current month
    uint8_t getMaxMonthDay() const;

    static uint8_t getDaysInMonth( uint8_t month );

    // @brief convert date to unix time duration
    ClockDuration toClockDuration() const;
    // @brief convert date to time point
    TimePoint toTimePoint() const;
    // @brief convert date to chrono seconds
    std::chrono::seconds toSeconds() const;
    // @brief convert date to human readable string
    // @param farmat definition of format. For detail infomation see decription of strftime (
    // http://www.cplusplus.com/reference/ctime/strftime/ )
    std::string toString( const std::string& format = "%FT%T%z" ) const;

private:
    Year m_year;
    Month m_month;
    Day m_day;
    Weekday m_weekday;
    Hour m_hour;
    Minute m_minute;
    Second m_second;
    Millisecond m_millisecond;
    std::chrono::seconds m_timeZoneOffset;
};

bool operator<( const Date& a, const Date& b );
bool operator<=( const Date& a, const Date& b );

}  // namespace GregorianCalendar
