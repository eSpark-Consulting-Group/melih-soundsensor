#include "TimezoneService.h"

#include "time_service/Timezone.h"
#include "Bus.h"
#include "BusCommandNmaRequestTimeZones.h"
#include "time_service/commands/SetTimezone.h"
#include "BinaryFileStorageFactory.h"
#include "StorageSection.h"
#include "StorageCommonSection.h"

#include "DebugUtils.h"

namespace time_service {

using namespace std::chrono_literals;

namespace {
constexpr std::uint32_t SecondsInDay = 24 * 60 * 60;
}  // namespace

bool TimezoneService::initializeImpl() {
    if( !core::Shutdownable::initializeImpl() ) {
        return false;
    }

    if( !bus::Bus::get().join( command::BUS_NAME, *this ) ) {
        FUNC_LOGE( "Can't join to %s namespace", command::BUS_NAME );
        return false;
    }

    if( !::bus::Bus::get().join( responce::BUS_NAME, *this ) ) {
        FUNC_LOGE( "Can't join to %s bus namespace", responce::BUS_NAME );
        return false;
    }

    if( !bus::Bus::get().join( cloud_client::responces::BUS_NAME, *this ) ) {
        FUNC_LOGE( "Can't join to bus namespace: %s", cloud_client::responces::BUS_NAME );
        return false;
    }

    {
        auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::COMMON );
        if( !store ) {
            return false;
        }

        std::int64_t expTime;
        m_expireTime = store->getInt64( storage::section::common::NMA_EXPIRE_TIME, expTime ) ? expTime : 0;
        FUNC_LOGD( "Set expTime = %lld", expTime );
    }

    return true;
}

TimezoneService::ClbkRet TimezoneService::updateComponent( UpdContext& ctx ) {
    if( !m_isConnectedToCloud || isDefaultTimezone() ) {
        return setDelay( 1s );
    }

    if( cnt_get_tz ) {
        cnt_get_tz -= 1;
        return setDelay( 1s );
    }

    using SysClock = std::chrono::system_clock;
    auto now = SysClock::now();
    auto t = SysClock::to_time_t( now );
    if( tz_update_timepoint == 0 || t > tz_update_timepoint ) {
        tz_update_timepoint = getTimeToCheckExpired();

        if( tz_update_timepoint == 0 ) {
            tz_update_timepoint = t + SecondsInDay / 2;  // 12h ahead
        }
        else {
            setNextTimeAtMidnight();
            tz_update_timepoint = getTimeToCheckExpired();
            FUNC_LOGW( "Updated tz_check timepoint: %llu", tz_update_timepoint );
        }

        cnt_get_tz = TZ_COUNTER;
        bus::sendFromNowhere<bus::command::nma::RequestTimeZones>( "nma", getTimeZoneId() );
    }

    cnt_get_tz = TZ_COUNTER;

    return setDelay( 1s );
}

void TimezoneService::visit( const bus::BusAddr, const bus::BusAddr, command::SetTimezone& cmd ) {
    if( !setTzEnvVar( cmd.timezoneId(), cmd.timezoneDescr() ) ) {
        FUNC_LOGE( "Cant set timezone with loc:%s, descr:%s", cmd.timezoneId().c_str(), cmd.timezoneDescr().c_str() );
    }

    auto expireTime = cmd.expireTime();
    if( expireTime ) {
        setExpireTime( *expireTime );
        FUNC_LOGI( "Expire Time update available,  expire_time=%ld", *expireTime );
    }
}

void TimezoneService::visit( const bus::BusAddr from,
                             const bus::BusAddr to,
                             const responce::TimeEstablished& command ) {
    time_t sntpTimeNow = {};
    time( &sntpTimeNow );

    tm sntpTimeInfo = {};
    localtime_r( &sntpTimeNow, &sntpTimeInfo );
    sntpTimeInfo.tm_hour = 0;
    sntpTimeInfo.tm_min = 0;
    sntpTimeInfo.tm_sec = 5;

    m_atmidnight = mktime( &sntpTimeInfo );
    m_tzSetDelay = TZ_DELAY;

    FUNC_LOGI( "Set time with delayed UpdRules: %d sec", m_tzSetDelay / 10 );
}

time_t TimezoneService::getTimeToCheckExpired() {
    if( m_expireTime && m_expireTime < m_atmidnight ) {
        return m_expireTime;
    }
    return m_atmidnight;
}

void TimezoneService::setExpireTime( const time_t expTime ) {
    m_expireTime = 0;
    auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::COMMON );
    if( !store ) {
        FUNC_LOGE( "Failed to open storage for NMA_EXPIRE_TIME" );
        return;
    }

    if( store->setInt64( storage::section::common::NMA_EXPIRE_TIME, expTime ) ) {
        m_expireTime = expTime;
    }
    else {
        FUNC_LOGE( "Failed to save expire time" );
    }
}

void TimezoneService::setNextTimeAtMidnight() {
    time_t next_check = time( NULL ) + SecondsInDay;
    tm next_midnight;
    localtime_r( &next_check, &next_midnight );
    next_midnight.tm_hour = 0;
    next_midnight.tm_min = 0;
    next_midnight.tm_sec = 5;
    m_atmidnight = mktime( &next_midnight );  // 24h ahead
    //    FUNC_LOGD( "Updated tne next time at midnight: %ld", m_atmidnight );
}

}  // namespace time_service
