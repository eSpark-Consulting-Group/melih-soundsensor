#pragma once

#include <string>
#include <ctime>

namespace core {

/// Represents a timer and it also offers methods for computing the next sunset and sunrise.
class Timer final {
public:
    enum TimerType {
        TimerInterval = 1,  ///< Interval timer
        TimerDayOfWeek,     ///< Days of week timer
        TimerDayOfMonth,    ///< Days of month timer
    };

public:
    static Timer& get();
    ~Timer() = default;

    double degToRad( const double angleDeg ) const;
    double radToDeg( const double angleRad ) const;

    double calcSunriseUTC( double JD, double latitude, double longitude );
    double calcJD( int year, int month, int day );
    double calcSunsetUTC( double JD, double latitude, double longitude );
    double calcEquationOfTime( const double t ) const;
    double calcSunDeclination( const double t ) const;
    double calcHourAngleSunset( double lat, double solarDec );
    double calcTimeJulianCent( const double jd ) const;
    double calcJDFromJulianCent( const double t ) const;
    double calcMeanObliquityOfEcliptic( const double t ) const;
    double calcSunEqOfCenter( const double t ) const;
    double calcGeomMeanLongSun( const double t ) const;
    double calcObliquityCorrection( const double t ) const;
    double calcEccentricityEarthOrbit( const double t ) const;
    double calcGeomMeanAnomalySun( const double t ) const;
    double calcSunTrueLong( const double t ) const;
    double calcSunApparentLong( const double t ) const;
    double calcHourAngleSunrise( const double lat, const double solarDec ) const;

    /// Computes the timestamp of the next sunrise and the next sunset
    bool GetSunriseSunset( int iYear, int iMonth, int iDay, float longitude, float latitude );
    time_t getSunrise() const;
    time_t getSunset() const;
    /// @name These functions return a 'next time' as a timestamp
    /// @{
    time_t GetTimeOnDay( struct tm* tm_Now, const char* sTime, float longitude, float latitude );
    time_t NextAbsoluteTime( uint16_t iYear, uint8_t iMonth, uint8_t iDay, const char* sTime, time_t tAtRun = 0 );
    time_t NextTime_Interval( const char* sTime, time_t tStart );
    time_t NextTime_DayOfWeek( const char* sTime, const char* sDays, float longitude, float latitude );
    time_t NextTime_DayOfMonth( const char* sTime, const char* sDays, float longitude, float latitude );
    time_t NextTime_Absolute( const char* sTime );
    std::string ISODateTime( time_t t = 0 ); /** < converts a time_t structure into a sql formatted date time */
    time_t NextTime( TimerType timerType, const char* sTime, const char* sDays, time_t tLastRun = 0 );
    /// @}
private:
    Timer();

    time_t m_sunrise;
    time_t m_sunset;
};

}  // namespace core
