#include "gregorian_calendar.h"
#include "Ezlo.h"
#include <vector>
#include <cstring>

namespace TimeUtils {

std::string timeStructToString( const tm& time, const std::string& format ) {
    constexpr const size_t DateTimeLen = 100;
    std::string buffer( DateTimeLen, '\0' );
    const auto timeLen = strftime( &buffer[0], buffer.capacity(), format.c_str(), &time );
    assert( timeLen <= DateTimeLen + 1 );
    if( timeLen == 0 ) {
        return std::string();
    }
    buffer.resize( strlen( buffer.c_str() ) );
    return buffer;
}

struct tm stringToTimeStruct( const std::string& time, const std::string& format ) {
    struct tm tm;
    memset( &tm, 0, sizeof( struct tm ) );
    strptime( time.c_str(), format.c_str(), &tm );
    return tm;
}

std::set<uint8_t> convertWeekdays( const std::vector<std::string>& weekDays ) {
    std::set<uint8_t> result;
    static std::map<const std::string, uint8_t> mappings{ { "monday", 1 },   { "tuesday", 2 }, { "wednesday", 3 },
                                                          { "thursday", 4 }, { "friday", 5 },  { "saturday", 6 },
                                                          { "sunday", 7 } };

    for( const auto& weekDay : weekDays ) {
        auto find_it = mappings.find( weekDay );
        if( find_it != mappings.end() ) {
            result.insert( find_it->second );
        }
    }

    return result;
}

GregorianCalendar::Date getDateWithModifiedTime( const GregorianCalendar::Date& date, const std::string& time ) {
    auto tm = ::TimeUtils::stringToTimeStruct( time, "%H:%M:%S" );

    GregorianCalendar::Date result( date );
    result.setHour( tm.tm_hour );
    result.setMinute( tm.tm_min );
    result.setSecond( tm.tm_sec );
    result.setMillisecond( 0 );

    return result;
}

GregorianCalendar::Date localTimeGetter() {
    auto ezlo = ::core::Ezlo::get();
    auto tm_gmtoff = std::chrono::seconds( 0 );
    if( ezlo ) {
        tm_gmtoff = std::chrono::seconds( ezlo->getTimeOffset() );
    }

    return ::GregorianCalendar::Date( std::chrono::system_clock::now(), tm_gmtoff );
}

}  // namespace TimeUtils

namespace GregorianCalendar {

Date::Date()
: m_year( 1 )
, m_month( m_year )
, m_day( m_month )
, m_weekday( m_day )
, m_hour( m_day )
, m_minute( m_hour )
, m_second( m_minute )
, m_millisecond( m_second )
, m_timeZoneOffset( 0 ) {
    m_month.setCurrentDayCallback( [this]() { return m_day.getUnit(); } );
}

Date::Date( const std::string& readableTime, const std::string& format )
: Date() {
    struct tm tm = TimeUtils::stringToTimeStruct( readableTime, format );
    if( tm.tm_wday == 0 ) {
        tm.tm_wday = 7;
    }
    m_weekday.setUnit( tm.tm_wday );  // order matters
    m_year.setUnit( tm.tm_year + 1900 );
    m_month.setUnit( tm.tm_mon + 1 );
    m_day.setUnit( tm.tm_mday );
    m_hour.setUnit( tm.tm_hour );
    m_minute.setUnit( tm.tm_min );
    m_second.setUnit( tm.tm_sec );
    m_millisecond.setUnit( 0 );
    auto ezlo = ::core::Ezlo::get();
    auto tm_gmtoff = std::chrono::seconds( 0 );
    if( ezlo ) {
        tm_gmtoff = std::chrono::seconds( ezlo->getTimeOffset() );
    }
    m_timeZoneOffset = tm_gmtoff;
}

Date::Date( const TimePoint& date, const std::chrono::seconds timeZoneOffset )
: m_year( date )
, m_month( m_year )
, m_day( m_month )
, m_weekday( m_day )
, m_hour( m_day )
, m_minute( m_hour )
, m_second( m_minute )
, m_millisecond( m_second )
, m_timeZoneOffset( timeZoneOffset ) {
    operator=( date + m_timeZoneOffset );
    m_month.setCurrentDayCallback( [this]() { return m_day.getUnit(); } );
}

Date::Date( const Date& date )
: m_year( date.getYear() )
, m_month( m_year )
, m_day( m_month )
, m_weekday( m_day )
, m_hour( m_day )
, m_minute( m_hour )
, m_second( m_minute )
, m_millisecond( m_second ) {
    operator=( date );
    m_month.setCurrentDayCallback( [this]() { return m_day.getUnit(); } );
}

Date& Date::operator=( const TimePoint& date ) {
    m_day.setUnit( 1 );
    m_year.setUnit( date );
    m_month.setUnit( date );
    m_day.setUnit( date );
    m_weekday.setUnit( date );
    m_hour.setUnit( date );
    m_minute.setUnit( date );
    m_second.setUnit( date );
    m_millisecond.setUnit( date );
    return *this;
}

Date& Date::operator=( const Date& date ) {
    m_year.setUnit( date.getYear() );
    m_month.setUnit( date.getMonth() );
    m_day.setUnit( date.getDay() );
    m_hour.setUnit( date.getHour() );
    m_minute.setUnit( date.getMinute() );
    m_second.setUnit( date.getSecond() );
    m_millisecond.setUnit( date.getMillisecond() );
    m_timeZoneOffset = date.m_timeZoneOffset;
    m_weekday.setUnit( TimePoint( toClockDuration() ) );
    return *this;
}

void Date::setYear( int64_t value ) {
    m_year.setUnit( value );
}

bool Date::setMonth( uint8_t value ) {
    return m_month.setUnit( value );
}

bool Date::setDay( uint8_t value ) {
    return m_day.setUnit( value );
}

bool Date::setWeekday( uint8_t value ) {
    return m_weekday.setUnit( value );
}

bool Date::setHour( uint8_t value ) {
    return m_hour.setUnit( value );
}

bool Date::setMinute( uint8_t value ) {
    return m_minute.setUnit( value );
}

bool Date::setSecond( uint8_t value ) {
    return m_second.setUnit( value );
}

bool Date::setMillisecond( uint16_t value ) {
    return m_millisecond.setUnit( value );
}

void Date::setTimeZoneOffset( const std::chrono::seconds& offset ) {
    m_timeZoneOffset = offset;
}

bool Date::moveToWeekday( uint8_t value ) {
    return m_weekday.moveToWeekday( value );
}

int64_t Date::getYear() const {
    return m_year.getUnit();
}

uint8_t Date::getMonth() const {
    return m_month.getUnit();
}

uint8_t Date::getDay() const {
    return m_day.getUnit();
}

uint8_t Date::getWeekday() const {
    return m_weekday.getUnit();
}

uint8_t Date::getHour() const {
    return m_hour.getUnit();
}

uint8_t Date::getMinute() const {
    return m_minute.getUnit();
}

uint8_t Date::getSecond() const {
    return m_second.getUnit();
}

uint16_t Date::getMillisecond() const {
    return m_millisecond.getUnit();
}

std::chrono::seconds Date::getTimeZoneOffset() const {
    return m_timeZoneOffset;
}

Date& Date::addYears( const Year& year ) {
    m_year += year;
    return *this;
}

Date& Date::addMonths( int64_t value ) {
    m_month += value;
    return *this;
}

Date& Date::addDays( int64_t value ) {
    m_day += value;
    return *this;
}

Date& Date::addWeekdays( int64_t value ) {
    m_weekday += value;
    return *this;
}

Date& Date::addHours( int64_t value ) {
    m_hour += value;
    return *this;
}

Date& Date::addMinutes( int64_t value ) {
    m_minute += value;
    return *this;
}

Date& Date::addSeconds( int64_t value ) {
    m_second += value;
    return *this;
}

Date& Date::addMilliseconds( int64_t value ) {
    m_millisecond += value;
    return *this;
}

Date& Date::incrementYear() {
    ++m_year;
    return *this;
}

Date& Date::incrementMonth() {
    ++m_month;
    return *this;
}

Date& Date::incrementDay() {
    ++m_day;
    return *this;
}

Date& Date::incrementWeek() {
    m_weekday.incrementWeek();
    return *this;
}

Date& Date::incrementHour() {
    ++m_hour;
    return *this;
}

Date& Date::incrementMinute() {
    ++m_minute;
    return *this;
}

Date& Date::incrementSecond() {
    ++m_second;
    return *this;
}

Date& Date::incrementMillisecond() {
    ++m_millisecond;
    return *this;
}

Date& Date::decrementYear() {
    --m_year;
    return *this;
}

Date& Date::decrementMonth() {
    --m_month;
    return *this;
}

Date& Date::decrementDay() {
    --m_day;
    return *this;
}

Date& Date::decrementWeek() {
    m_weekday.decrementWeek();
    return *this;
}

Date& Date::decrementHour() {
    --m_hour;
    return *this;
}

Date& Date::decrementMinute() {
    --m_minute;
    return *this;
}

Date& Date::decrementSecond() {
    --m_second;
    return *this;
}

Date& Date::decrementMillisecond() {
    --m_millisecond;
    return *this;
}

uint8_t Date::getMaxMonthDay() const {
    return m_month.getDaysInCurrentMonth();
}

uint8_t Date::getDaysInMonth( uint8_t month ) {
    return Month::getDaysInMonth( month );
}

ClockDuration Date::toClockDuration() const {
    return m_millisecond.totalDuration() - m_timeZoneOffset;
}

TimePoint Date::toTimePoint() const {
    return TimePoint( toClockDuration() );
}

std::chrono::seconds Date::toSeconds() const {
    return std::chrono::duration_cast<std::chrono::seconds>( toClockDuration() );
}

std::string Date::toString( const std::string& format ) const {
    struct tm timeInfo;
    auto now = time( nullptr );
    localtime_r( &now, &timeInfo );
    timeInfo.tm_sec = getSecond();
    timeInfo.tm_min = getMinute();
    timeInfo.tm_hour = getHour();
    timeInfo.tm_mday = getDay();
    timeInfo.tm_mon = getMonth() - 1;
    timeInfo.tm_year = getYear() - 1900;
    return TimeUtils::timeStructToString( timeInfo, format );
}

Year::Year( const TimePoint& sinceEpoch ) {
    setUnit( sinceEpoch );
}

Year::Year( int64_t year ) {
    setUnit( year );
}

void Year::setUnit( int64_t year ) {
    m_leapYears = std::chrono::leap_years( getLeapYearsNumber( year ) );
    m_regularYears = std::chrono::years( year - m_leapYears.count() );
}

void Year::setUnit( const TimePoint& sinceEpoch ) {
    using namespace std::chrono;
    int64_t yearsSinceAnnoDomini = getUnit( sinceEpoch );
    m_leapYears = leap_years( getLeapYearsNumber( yearsSinceAnnoDomini ) );
    m_regularYears = years( yearsSinceAnnoDomini - m_leapYears.count() );
}

Year& Year::operator++() {
    int64_t year = getUnit();
    ++year;
    if( year == 0 ) {
        ++m_regularYears;
    }
    if( year > 0 ) {
        --year;
    }
    if( isLeapYear( year ) ) {
        ++m_leapYears;
    }
    else {
        ++m_regularYears;
    }
    return *this;
}

Year& Year::operator--() {
    int64_t year = getUnit();
    --year;
    if( year == 0 ) {
        --m_regularYears;
    }
    if( year < 0 ) {
        ++year;
    }
    if( isLeapYear( year ) ) {
        --m_leapYears;
    }
    else {
        --m_regularYears;
    }
    return *this;
}

Year& Year::operator+=( const Year& value ) {
    m_regularYears += value.m_regularYears;
    m_leapYears += value.m_leapYears;
    return *this;
}

Year& Year::operator-=( const Year& value ) {
    m_regularYears -= value.m_regularYears;
    m_leapYears -= value.m_leapYears;
    return *this;
}

bool Year::isLeapYear() const {
    return isLeapYear( getUnit() );
}

int64_t Year::getLeapYearsNumber() {
    return m_leapYears.count();
}

int64_t Year::getRegularYearsNumber() {
    return m_regularYears.count();
}

bool Year::isLeapYear( int64_t year ) {
    if( year == 0 ) {
        return false;
    }
    return ( ( ( year % 4 == 0 ) && ( year % 100 != 0 ) ) || ( year % 400 == 0 ) );
}

int64_t Year::getLeapYearsNumber( int64_t year ) {
    if( year > 0 ) {
        --year;
    }
    else {
        ++year;
    }
    return year / 4 - year / 100 + year / 400;
}

int64_t Year::getUnit( const TimePoint& sinceEpoch ) {
    using namespace std::chrono;
    days epochDaysSinceAnnoDomini( EpochDaysSinceAnnoDomini );
    days daysSinceAnnoDomini = duration_cast<days>( sinceEpoch.time_since_epoch() ) + epochDaysSinceAnnoDomini;
    int64_t yearsSinceAnnoDomini = daysSinceAnnoDomini.count() / DaysInYear + 0.001;
    return yearsSinceAnnoDomini;
}

int64_t Year::getUnit() const {
    if( m_regularYears.count() == 0 ) {
        return m_leapYears.count() + m_regularYears.count() - 1;
    }
    return m_leapYears.count() + m_regularYears.count();
}

ClockDuration Year::toClockDuration() const {
    auto epochLeapYears = std::chrono::leap_years( getLeapYearsNumber( EpochYear ) );
    auto epochRegularYears = std::chrono::years( EpochYear - epochLeapYears.count() );
    return m_regularYears + m_leapYears - epochLeapYears - epochRegularYears;
}

const std::vector<uint8_t> Month::daysNumberInMonth = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

Month::Month( Year& year )
: m_year( year )
, m_month( 1 )
, m_getCurrentDay( []() { return 1; } ) {}

void Month::setUnit( const TimePoint& sinceEpoch ) {
    using namespace std::chrono;
    m_month = 1;
    days leftDays = duration_cast<days>( sinceEpoch.time_since_epoch() - m_year.toClockDuration() );
    for( ; leftDays >= days( 0 ); leftDays -= days( getDaysInCurrentMonth() ), ++( *this ) ) {
    }
    --( *this );
}

bool Month::setUnit( uint8_t value ) {
    if( value < 1 || value > MonthsNumberInYear ) {
        return false;
    }
    m_month = value;
    return true;
}

void Month::setCurrentDayCallback( Month::GetCurrentDay callback ) {
    m_getCurrentDay = callback;
}

uint8_t Month::getUnit() const {
    return m_month;
}

uint8_t Month::getDaysInCurrentMonth() const {
    const static std::vector<uint8_t> daysNumberInMonth = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if( m_month == 2 && m_year.isLeapYear() ) {
        return daysNumberInMonth[m_month - 1] + 1;
    }
    return daysNumberInMonth[m_month - 1];
}

std::chrono::days Month::getDaysFromYearBeginning() const {
    const static std::vector<uint16_t> daysNumberFromYearBeginning = { 0,   31,  59,  90,  120, 151,
                                                                       181, 212, 243, 273, 304, 334 };
    if( m_month > 2 && m_year.isLeapYear() ) {
        return std::chrono::days( daysNumberFromYearBeginning[m_month - 1] + 1 );
    }
    return std::chrono::days( daysNumberFromYearBeginning[m_month - 1] );
}

ClockDuration Month::toClockDuration() const {
    return getDaysFromYearBeginning();
}

ClockDuration Month::totalDuration() const {
    return m_year.toClockDuration() + toClockDuration();
}

uint8_t Month::getDaysInMonth( uint8_t month ) {
    if( month < 1 || month > 12 ) {
        return 0;
    }
    return daysNumberInMonth[month - 1];
}

Month& Month::operator++() {
    do {
        ++m_month;
        if( m_month > MonthsNumberInYear ) {
            m_month = 1;
            ++m_year;
        }
    } while( m_getCurrentDay() > getDaysInCurrentMonth() );
    return *this;
}

Month& Month::operator--() {
    do {
        --m_month;
        if( m_month == 0 ) {
            m_month = MonthsNumberInYear;
            --m_year;
        }
    } while( m_getCurrentDay() > getDaysInCurrentMonth() );
    return *this;
}

Month& Month::operator+=( int64_t months ) {
    if( m_getCurrentDay() < 29 ) {
        int64_t sum = m_month + months;
        m_month = std::abs( sum % MonthsNumberInYear );
        if( m_month == 0 ) {
            m_month = MonthsNumberInYear;
            if( months < 0 ) {
                m_year -= Year( 1 );
            }
        }
        else {
            m_year += Year( sum / MonthsNumberInYear );
        }
    }
    else {
        bool isIncrement = months > 0;
        for( ; months != 0; ( isIncrement ) ? --months : ++months ) {
            ( isIncrement ) ? operator++() : operator--();
        }
    }
    return *this;
}

Month& Month::operator-=( int64_t months ) {
    operator+=( -months );
    return *this;
}

Day::Day( Month& month )
: m_month( month )
, m_day( 1 ) {}

void Day::setUnit( const TimePoint& time ) {
    ClockDuration leftTime = time.time_since_epoch() - m_month.totalDuration();
    int64_t days = leftTime / std::chrono::days( 1 ) + 1;
    uint8_t maxDays = m_month.getDaysInCurrentMonth();
    if( days > maxDays ) {
        days %= maxDays;
    }
    m_day = days;
}

bool Day::setUnit( uint8_t value ) {
    uint8_t maxDays = m_month.getDaysInCurrentMonth();
    if( value < 1 || value > maxDays ) {
        return false;
    }
    m_day = value;
    return true;
}

uint8_t Day::getUnit() const {
    return m_day;
}

ClockDuration Day::toClockDuration() const {
    return std::chrono::days( m_day - 1 );
}

ClockDuration Day::totalDuration() const {
    return m_month.totalDuration() + toClockDuration();
}

Day& Day::operator++() {
    uint8_t maxDays = m_month.getDaysInCurrentMonth();
    ++m_day;
    if( m_day > maxDays ) {
        m_day = 1;
        ++m_month;
    }
    return *this;
}

Day& Day::operator--() {
    --m_day;
    if( m_day == 0 ) {
        --m_month;
        m_day = m_month.getDaysInCurrentMonth();
    }
    return *this;
}

Day& Day::operator+=( int64_t days ) {
    days += m_day;
    bool isIncrement = days > 0;
    days = std::abs( days );
    m_day = 1;
    for( ; days > m_month.getDaysInCurrentMonth(); ) {
        if( isIncrement ) {
            days -= m_month.getDaysInCurrentMonth();
            ++m_month;
        }
        else {
            --m_month;
            days -= m_month.getDaysInCurrentMonth();
        }
    }
    if( days == 0 ) {
        if( isIncrement ) {
            ++m_month;
        }
        else {
            --m_month;
        }
        days = m_month.getDaysInCurrentMonth();
    }
    else if( !isIncrement ) {
        --m_month;
        m_day = m_month.getDaysInCurrentMonth() - days;
        return *this;
    }
    m_day = days;
    return *this;
}

Day& Day::operator-=( int64_t days ) {
    operator+=( -days );
    return *this;
}

Weekday::Weekday( Day& day )
: m_day( day )
, m_weekday( 1 ) {}

void Weekday::setUnit( const TimePoint& time ) {
    using namespace std::chrono;
    days daysSinceEpoch = duration_cast<days>( time.time_since_epoch() );
    m_weekday = ( daysSinceEpoch.count() + WeekdayInEpoch ) % DaysInWeek;
    if( m_weekday == 0 ) {
        m_weekday = DaysInWeek;
    }
    if( m_weekday < 0 ) {
        m_weekday += DaysInWeek;
    }
}

bool Weekday::setUnit( uint8_t value ) {
    if( isWeekdayValid( value ) ) {
        return false;
    }
    m_weekday = value;
    return true;
}

uint8_t Weekday::getUnit() const {
    return m_weekday;
}

bool Weekday::moveToWeekday( uint8_t value ) {
    if( isWeekdayValid( value ) ) {
        return false;
    }
    int8_t diff = static_cast<int8_t>( value ) - m_weekday;
    operator+=( diff );
    return true;
}

Weekday& Weekday::incrementWeek() {
    return operator+=( DaysInWeek );
}

Weekday& Weekday::decrementWeek() {
    return operator-=( DaysInWeek );
}

Weekday& Weekday::operator++() {
    ++m_day;
    ++m_weekday;
    if( m_weekday == DaysInWeek + 1 ) {
        m_weekday = 1;
    }
    return *this;
}

Weekday& Weekday::operator--() {
    --m_day;
    --m_weekday;
    if( m_weekday == 0 ) {
        m_weekday = 7;
    }
    return *this;
}

Weekday& Weekday::operator+=( int64_t days ) {
    m_day += days;
    int64_t weekdays = m_weekday + days;
    weekdays %= DaysInWeek;
    if( weekdays == 0 ) {
        weekdays = 7;
    }
    if( weekdays < 0 ) {
        weekdays += DaysInWeek;
    }
    m_weekday = weekdays;
    return *this;
}

Weekday& Weekday::operator-=( int64_t days ) {
    return operator+=( -days );
}

bool Weekday::isWeekdayValid( uint8_t value ) {
    return value < 1 or value > DaysInWeek;
}

bool operator<( const Date& a, const Date& b ) {
    return a.toClockDuration() < b.toClockDuration();
}

bool operator<=( const Date& a, const Date& b ) {
    auto aDuration = a.toClockDuration();
    auto bDuration = b.toClockDuration();

    return ( aDuration < bDuration ) || ( aDuration == bDuration );
}

}  // namespace GregorianCalendar
