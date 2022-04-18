#include <Timer.h>
#include "hub_core/GeoSettings.h"

#include <string.h>
#include <cctype>
#include <algorithm>
#include <time.h>
#include <math.h>
#include <iomanip>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
#define MAX_SUNSET_SUNRISE_OFFSET 605000
// If we're more than this many seconds +/- current time, assume the sunset/sunrise is invalid

//#define ENABLE_TMR_LOGS
#ifdef ENABLE_TMR_LOGS

#include <DebugUtils.h>

#define TMR_LOG_TAG "   -----+++++ [Timer] "
#define TMR_LOGI( _FMT, ... ) ESP_LOGI( TMR_LOG_TAG, _FMT, ##__VA_ARGS__ )
#define TMR_LOGE( _FMT, ... ) ESP_LOGE( TMR_LOG_TAG, _FMT, ##__VA_ARGS__ )
#define TMR_LOGW( _FMT, ... ) ESP_LOGW( TMR_LOG_TAG, _FMT, ##__VA_ARGS__ )

#else

#define TMR_LOGI( ... ) \
    do {                \
    } while( false )
#define TMR_LOGE( ... ) \
    do {                \
    } while( false )
#define TMR_LOGW( ... ) \
    do {                \
    } while( false )

#endif
namespace core {

Timer::Timer()
: m_sunrise( 0 )
, m_sunset( 0 ) {}

Timer& Timer::get() {
    static Timer timer;
    return timer;
}

double Timer::degToRad( const double angleDeg ) const {
    return ( M_PI * angleDeg / 180.0 );
}

double Timer::radToDeg( const double angleRad ) const {
    return ( 180.0 * angleRad / M_PI );
}

double Timer::calcMeanObliquityOfEcliptic( const double t ) const {
    double seconds = 21.448 - t * ( 46.8150 + t * ( 0.00059 - t * ( 0.001813 ) ) );

    return ( 23.0 + ( 26.0 + ( seconds / 60.0 ) ) / 60.0 );  // in degrees
}

double Timer::calcGeomMeanLongSun( const double t ) const {
    double L = 280.46646 + t * ( 36000.76983 + 0.0003032 * t );

    double normalizeAngle = fmod( L, 360 );
    if( normalizeAngle < 0 ) {
        normalizeAngle += 360;
    }
    // TMR_LOGD( "Timer: calcGeomMeanLongSun for angle L(%g), normang(%g)\n", L, normalizeAngle );

    return normalizeAngle;  // in degrees
}

double Timer::calcObliquityCorrection( const double t ) const {
    double e0 = calcMeanObliquityOfEcliptic( t );
    double omega = 125.04 - 1934.136 * t;

    return ( e0 + 0.00256 * cos( degToRad( omega ) ) );  // in degrees
}

double Timer::calcEccentricityEarthOrbit( const double t ) const {
    return ( 0.016708634 - t * ( 0.000042037 + 0.0000001267 * t ) );  // unitless
}

double Timer::calcGeomMeanAnomalySun( const double t ) const {
    return ( 357.52911 + t * ( 35999.05029 - 0.0001537 * t ) );  // in degrees
}

double Timer::calcEquationOfTime( const double t ) const {
    double epsilon = calcObliquityCorrection( t );
    double l0 = calcGeomMeanLongSun( t );
    double e = calcEccentricityEarthOrbit( t );
    double m = calcGeomMeanAnomalySun( t );
    double y = tan( degToRad( epsilon ) / 2.0 );
    y *= y;
    double sin2l0 = sin( 2.0 * degToRad( l0 ) );
    double sinm = sin( degToRad( m ) );
    double cos2l0 = cos( 2.0 * degToRad( l0 ) );
    double sin4l0 = sin( 4.0 * degToRad( l0 ) );
    double sin2m = sin( 2.0 * degToRad( m ) );
    double Etime =
        y * sin2l0 - 2.0 * e * sinm + 4.0 * e * y * sinm * cos2l0 - 0.5 * y * y * sin4l0 - 1.25 * e * e * sin2m;

    return ( radToDeg( Etime ) * 4.0 );  // in minutes of time
}

double Timer::calcTimeJulianCent( const double jd ) const {
    return ( ( jd - 2451545.0 ) / 36525.0 );
}

double Timer::calcSunTrueLong( const double t ) const {
    double l0 = calcGeomMeanLongSun( t );
    double c = calcSunEqOfCenter( t );

    return ( l0 + c );  // in degrees
}

double Timer::calcSunApparentLong( const double t ) const {
    double o = calcSunTrueLong( t );

    double omega = 125.04 - 1934.136 * t;
    double lambda = o - 0.00569 - 0.00478 * sin( degToRad( omega ) );
    return lambda;  // in degrees
}

double Timer::calcSunDeclination( const double t ) const {
    double e = calcObliquityCorrection( t );
    double lambda = calcSunApparentLong( t );
    double sint = sin( degToRad( e ) ) * sin( degToRad( lambda ) );

    return radToDeg( asin( sint ) );  // in degrees
}

double Timer::calcHourAngleSunrise( const double lat, const double solarDec ) const {
    double latRad = degToRad( lat );
    double sdRad = degToRad( solarDec );

    double HA = ( acos( cos( degToRad( 90.833 ) ) / ( cos( latRad ) * cos( sdRad ) ) - tan( latRad ) * tan( sdRad ) ) );

    return HA;  // in radians
}

double Timer::calcHourAngleSunset( double lat, double solarDec ) {
    double latRad = degToRad( lat );
    double sdRad = degToRad( solarDec );

    double HA = ( acos( cos( degToRad( 90.833 ) ) / ( cos( latRad ) * cos( sdRad ) ) - tan( latRad ) * tan( sdRad ) ) );

    return -HA;  // in radians
}

double Timer::calcJD( int year, int month, int day ) {
    if( month <= 2 ) {
        year -= 1;
        month += 12;
    }
    int A = floor( (double)year / 100 );
    int B = 2 - A + floor( (double)A / 4 );

    double JD = floor( 365.25 * ( year + 4716 ) ) + floor( 30.6001 * ( month + 1 ) ) + day + B - 1524.5;
    return JD;
}

double Timer::calcJDFromJulianCent( const double t ) const {
    return t * 36525.0 + 2451545.0;
}

double Timer::calcSunEqOfCenter( const double t ) const {
    double m = calcGeomMeanAnomalySun( t );

    double mrad = degToRad( m );
    double sinm = sin( mrad );
    double sin2m = sin( mrad + mrad );
    double sin3m = sin( mrad + mrad + mrad );

    double C =
        sinm * ( 1.914602 - t * ( 0.004817 + 0.000014 * t ) ) + sin2m * ( 0.019993 - 0.000101 * t ) + sin3m * 0.000289;
    return C;  // in degrees
}

double Timer::calcSunriseUTC( double JD, double latitude, double longitude ) {
    double t = calcTimeJulianCent( JD );

    // *** First pass to approximate sunrise
    double eqTime = calcEquationOfTime( t );
    double solarDec = calcSunDeclination( t );
    double hourAngle = calcHourAngleSunrise( latitude, solarDec );
    double delta = longitude + radToDeg( hourAngle );
    double timeDiff = 4 * delta;               // in minutes of time
    double timeUTC = 720 - timeDiff - eqTime;  // in minutes
    double newt = calcTimeJulianCent( calcJDFromJulianCent( t ) + timeUTC / 1440.0 );

    eqTime = calcEquationOfTime( newt );
    solarDec = calcSunDeclination( newt );

    hourAngle = calcHourAngleSunrise( latitude, solarDec );
    delta = longitude + radToDeg( hourAngle );
    timeDiff = 4 * delta;
    timeUTC = 720 - timeDiff - eqTime;  // in minutes

    return timeUTC;
}

double Timer::calcSunsetUTC( double JD, double latitude, double longitude ) {
    double t = calcTimeJulianCent( JD );
    // *** First pass to approximate sunset

    double eqTime = calcEquationOfTime( t );
    double solarDec = calcSunDeclination( t );
    double hourAngle = calcHourAngleSunset( latitude, solarDec );
    double delta = longitude + radToDeg( hourAngle );
    double timeDiff = 4 * delta;               // in minutes of time
    double timeUTC = 720 - timeDiff - eqTime;  // in minutes
    double newt = calcTimeJulianCent( calcJDFromJulianCent( t ) + timeUTC / 1440.0 );

    eqTime = calcEquationOfTime( newt );
    solarDec = calcSunDeclination( newt );

    hourAngle = calcHourAngleSunset( latitude, solarDec );
    delta = longitude + radToDeg( hourAngle );
    timeDiff = 4 * delta;
    timeUTC = 720 - timeDiff - eqTime;  // in minutes

    return timeUTC;
}

bool Timer::GetSunriseSunset( int iYear, int iMonth, int iDay, float longitude, float latitude ) {
    double JD = calcJD( iYear, iMonth, iDay );
    // TMR_LOGI( "Get:  Julian date = %g\n", JD );
    time_t seconds = 0;
    time_t tseconds = 0;
    struct tm tm;

    tm.tm_year = iYear - 1900;
    tm.tm_mon = iMonth - 1; /* Jan = 0, Feb = 1,.. Dec = 11 */
    tm.tm_mday = iDay;
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    tm.tm_isdst = 1;  /// DST - in effect //0 ///DST not in effect// //was -1;  ///DST unknown

    seconds = mktime( &tm );
    time_t seconds_at_midnight = seconds;

    struct tm ptm;
    gmtime_r( &seconds, &ptm );
    int delta = ptm.tm_hour;

    tseconds = seconds;
    seconds += calcSunriseUTC( JD, latitude, longitude ) * 60;
    m_sunrise = seconds - delta * 3600;
    seconds = tseconds;
    seconds += calcSunsetUTC( JD, latitude, longitude ) * 60;
    m_sunset = seconds - delta * 3600;
    if( m_sunrise < seconds_at_midnight || m_sunset < seconds_at_midnight ) {
        tseconds += 86400;
        JD += 1;

        seconds = tseconds;
        seconds = seconds + calcSunriseUTC( JD, latitude, longitude ) * 60;
        if( m_sunrise < seconds_at_midnight ) {
            m_sunrise = seconds - delta * 3600;
        }

        seconds = tseconds;
        seconds += calcSunsetUTC( JD, latitude, longitude ) * 60;
        if( m_sunset < seconds_at_midnight ) {
            m_sunset = seconds - delta * 3600;
        }
    }
    // Or doesn't return the next sunrise/sunset if we're close
    else if( m_sunrise >= seconds_at_midnight + 86400 || m_sunset >= seconds_at_midnight + 86400 ) {
        tseconds -= 86400;
        JD -= 1;

        seconds = tseconds;
        seconds = seconds + calcSunriseUTC( JD, latitude, longitude ) * 60;
        if( m_sunrise >= seconds_at_midnight + 86400 ) {
            m_sunrise = seconds - delta * 3600;
        }

        seconds = tseconds;
        seconds += calcSunsetUTC( JD, latitude, longitude ) * 60;
        if( m_sunset >= seconds_at_midnight + 86400 ) {
            m_sunset = seconds - delta * 3600;
        }
    }

    return true;
}

time_t Timer::getSunrise() const {
    return m_sunrise;
}

time_t Timer::getSunset() const {
    return m_sunset;
}

//-----------------------------------------------------------------------------
time_t Timer::GetTimeOnDay( struct tm* tm_Now, const char* sTime, float longitude, float latitude ) {
    bool bGotP = false, bGotR = false, bGotT = false, bNegative = false;
    int iTime[3] = { 0, 0, 0 };

    const char* p = sTime;
    int i = 0;

    while( *p && ( ( i < 3 ) && ( *p != '\0' ) ) ) {
        if( *p >= '0' && *p <= '9' ) {
            iTime[i] = atoi( p++ );
            while( *p && ( *p >= '0' && *p <= '9' ) ) {
                p++;  // Skip over any remaining numbers
            }
            i++;  // Don't just break because we need to pickup trailing r/p/t and cycle until the end
        }
        if( *p == '-' ) {
            bNegative = true;
        }
        if( *p == 'r' || *p == 'R' ) {
            bGotR = true;
        }
        if( *p == 'p' || *p == 'P' ) {
            bGotP = true;
        }
        if( *p == 't' || *p == 'T' ) {
            bGotT = true;
        }
        p++;
    }

    // If pm, add 12 hours
    if( bGotP ) {
        iTime[0] += 12;
    }

    // if sunrise/sunset
    if( bGotR || bGotT ) {
        int iDiff = iTime[2] + iTime[1] * 60 + iTime[0] * 3600;  // The offset
        if( bNegative ) {
            iDiff *= -1;
        }
        time_t tNow = mktime( tm_Now );
        time_t tOldestDate = tNow - MAX_SUNSET_SUNRISE_OFFSET;
        time_t tNewestDate = tNow + MAX_SUNSET_SUNRISE_OFFSET;

        if( GetSunriseSunset( tm_Now->tm_year + 1900, tm_Now->tm_mon + 1, tm_Now->tm_mday, longitude, latitude ) ) {
            if( bGotR && m_sunrise > tOldestDate && m_sunrise < tNewestDate ) {
                // TMR_LOGI( "Sunrise: GetTimeOnDay calculates sunrise=%ld, od=%ld,nd=%ld, off=%d\n",
                //           m_sunrise, tOldestDate, tNewestDate, iDiff );
                return ( m_sunrise + iDiff );
            }
            else if( bGotT && m_sunset > tOldestDate && m_sunset < tNewestDate ) {
                // TMR_LOGI( "Sunset: GetTimeOnDay calculates sunrise=%ld, od=%ld, nd=%ld, off=%d\n",
                //           m_sunset, tOldestDate, tNewestDate, iDiff );
                return ( m_sunset + iDiff );
            }
        }
        TMR_LOGE( "Sunstate: GetTimeOnDay can't calculate sunrise=%d, sunset=%d for %d-%d-%d long=%f lat=%f\n",
                  (int)m_sunrise,
                  (int)m_sunset,
                  tm_Now->tm_year + 1900,
                  tm_Now->tm_mon + 1,
                  tm_Now->tm_mday,
                  longitude,
                  latitude );
        return 0;
    }

    tm_Now->tm_hour = iTime[0];
    tm_Now->tm_min = iTime[1];
    tm_Now->tm_sec = iTime[2];
    // TMR_LOGI( "Time: GetTimeOnDay - hours:%d minutes:%d, seconds%d\n", iTime[0], iTime[1], iTime[2] );
    return mktime( tm_Now );
}

time_t Timer::NextTime_Interval( const char* sTime, time_t tStart ) {
    if( tStart == 0 ) {
        tStart = time( NULL );
    }

    int iOffset = atoi( sTime );
    if( iOffset < 1 ) {
        return 0;
    }

    if( strchr( sTime, 'm' ) != NULL ) {
        return ( tStart + iOffset * 60 );
    }
    else if( strchr( sTime, 'h' ) != NULL ) {
        return ( tStart + iOffset * 3600 );
    }
    else if( strchr( sTime, 'd' ) != NULL ) {
        return ( tStart + iOffset * 3600 * 24 );
    }
    return ( tStart + iOffset );
}

time_t Timer::NextTime_DayOfWeek( const char* sTime, const char* sDays, float longitude, float latitude ) {
    // Days is 1==Monday, 7==Sunday

    // It's possible that the time for today has already passed.  So we will need to
    // keep going forward until we get a date/time in the future.  8 days is the theoretical
    // max that we would need to check, so do an 8 day check, but iterate up to 9 times
    // because we only add 23 hours each time in case we cross a time change, so, theoretically
    // one day could be checked twice
    time_t tStart = time( NULL );
    time_t tNow = time( NULL );
    // TMR_LOGI( "Try NT_DOW tNow: %ld, tStart: %ld\n", tNow, tStart );
    for( int i = 0; i < 9; ++i ) {
        struct tm tm_Now;
        localtime_r( &tStart, &tm_Now );
        char CurrentDow = tm_Now.tm_wday == 0 ? 7 : tm_Now.tm_wday;
        // See if the day in tStart is one of our valid choices
        if( sDays == NULL || sDays[0] == 0 || strchr( sDays, CurrentDow + '0' ) != NULL ) {
            // Get the time on this day
            time_t tTime = GetTimeOnDay( &tm_Now, sTime, longitude, latitude );
            if( tTime > tNow ) {
                // TMR_LOGI( "NextTime for DOW gives check_time=%ld\n", tTime );
                return tTime;
            }
        }

        // Either the if above matched, and the time has already passed, or the
        // date in tStart isn't one on our list.  Add 23 hours so that in
        // case we've got a time zone change there's no possible way we'll skip a day
        tStart += 3600 * 23;
    }
    return 0;  // We didn't find anything
}

time_t Timer::NextTime_DayOfMonth( const char* sTime, const char* sDays, float longitude, float latitude ) {
    // Same as before, but use 32 for the iterator since we're doing days of the month
    time_t tStart = time( NULL );
    time_t tNow = time( NULL );
    for( int i = 0; i < 32; ++i ) {
        struct tm tm_Now;
        localtime_r( &tStart, &tm_Now );
        char sDay[5], sBeginning[5], sEnd[5], sMiddle[5];
        sprintf( sDay, "%d", tm_Now.tm_mday );
        sprintf( sBeginning, "%d,", tm_Now.tm_mday );
        sprintf( sEnd, ",%d", tm_Now.tm_mday );
        sprintf( sMiddle, ",%d,", tm_Now.tm_mday );
        const char* pEndPos = sDays ? sDays + strlen( sDays ) - strlen( sEnd ) : NULL;

        if( sDays == NULL || sDays[0] == 0 || strcmp( sDays, sDay ) == 0 ||  // Exact match on this single day
            strstr( sDays, sBeginning ) == sDays ||                          // This day is at the beginning of the list
            strstr( sDays, sMiddle ) != NULL ||  // This day is in the middle or day is at the end
            strstr( sDays, sEnd ) == pEndPos ) {
            // It is.  Get the time on this day
            time_t tTime = GetTimeOnDay( &tm_Now, sTime, longitude, latitude );
            if( tTime > tNow ) {
                // TMR_LOGI( "NextTime_DayOfMonth returnes check_time=%ld\n", tTime );
                return tTime;
            }
        }
        // Either the if above matched, and the time has already passed, or the
        // date in tStart isn't one on our list.  Add 23 hours so that in
        // case we've got a time zone change there's no possible way we'll skip a day
        tStart += 3600 * 23;
    }
    return 0;  // We didn't find anything
}

time_t Timer::NextAbsoluteTime( uint16_t iYear, uint8_t iMonth, uint8_t iDay, const char* sTime, time_t tAtRun ) {
    int iTime[2] = { 0, 0 };

    const char* p = sTime;
    int i = 0;
    while( *p && i < 2 ) {
        if( *p >= '0' && *p <= '9' ) {
            iTime[i] = atoi( p++ );
            while( *p && ( *p >= '0' && *p <= '9' ) ) {
                p++;
            }
            i++;
        }
        p++;
    }

    tm t;
    t.tm_year = iYear - 1900;
    t.tm_mon = iMonth - 1;
    t.tm_mday = iDay;
    t.tm_hour = iTime[0];
    t.tm_min = iTime[1];
    t.tm_sec = 0;
    t.tm_isdst = -1;
    time_t tReturn = mktime( &t );
    if( tReturn > tAtRun ) {
        return tReturn;
    }
    else {
        return 0;
    }
}

time_t Timer::NextTime( TimerType timerType, const char* sTime, const char* sDays, time_t tLastRun ) {
    auto [latitude, longitude] = core::geo_settings::getCoordinates();
    switch( timerType ) {
    case TimerInterval:
        return NextTime_Interval( sTime, tLastRun );
    case TimerDayOfWeek: {
        return NextTime_DayOfWeek( sTime, sDays, longitude, latitude );
    }
    case TimerDayOfMonth: {
        return NextTime_DayOfMonth( sTime, sDays, longitude, latitude );
    }
    }  // end switch

    TMR_LOGE( "NextTime timerType not valid, returning tTime=0 type=%d\n", (int)timerType );
    return 0;
}

std::string Timer::ISODateTime( time_t t ) {
    struct tm* lt = localtime( &t );
    char acDateTime[32] = {};
    strftime( acDateTime, sizeof( acDateTime ), "%FT%T%z", lt );
    return acDateTime;
}

}  // namespace core
