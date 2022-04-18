#include "Time.h"
#include "hub_core/GeoSettings.h"
#include "Timer.h"

#include "DebugUtils.h"

namespace scenes {

TimeOfDay getSunriseOfDay( const TimeInfo& timeInfo ) {
    auto [ltt, lnt] = core::geo_settings::getCoordinates();
    auto& tmr = core::Timer::get();

    if( !tmr.GetSunriseSunset( getYear( timeInfo ), getMonth( timeInfo ), getDayOfMonth( timeInfo ), lnt, ltt ) ) {
        return TimeOfDay();
    }
    auto sunrizeTt = tmr.getSunrise();
    auto sunrizeInfo = *std::localtime( &sunrizeTt );
    return TimeOfDay( sunrizeInfo );
}

TimeOfDay getSunsetOfDay( const TimeInfo& timeInfo ) {
    auto [ltt, lnt] = core::geo_settings::getCoordinates();
    auto& tmr = core::Timer::get();

    if( !tmr.GetSunriseSunset( getYear( timeInfo ), getMonth( timeInfo ), getDayOfMonth( timeInfo ), lnt, ltt ) ) {
        return TimeOfDay();
    }
    auto sunrizeTt = tmr.getSunset();
    auto sunsetInfo = *std::localtime( &sunrizeTt );
    return TimeOfDay( sunsetInfo );
}

}  // end of namespace scenes
