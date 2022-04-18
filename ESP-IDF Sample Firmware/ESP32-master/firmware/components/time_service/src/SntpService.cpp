#include "SntpService.h"
#include "Thread.h"
#include "time_service/responces/TimeEstablished.h"
#include "wifi_service2/WifiCommands.h"
#include "wifi_service2/Common.h"

#include <lwip/apps/sntp.h>

// #define ENABLE_SNTP_SRV_LOGS
#ifdef ENABLE_SNTP_SRV_LOGS

#include "DebugUtils.h"

#define SNTP_SRV_LOG_TAG "[SNTP_SRV"
#define SNTP_SRV_LOGI( _FMT, ... ) ESP_LOGI( SNTP_SRV_LOG_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )
#define SNTP_SRV_LOGE( _FMT, ... ) ESP_LOGE( SNTP_SRV_LOG_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )
#define SNTP_SRV_LOGW( _FMT, ... ) ESP_LOGW( SNTP_SRV_LOG_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )

#else  // not ENABLE_SNTP_SRV_LOGS

#define SNTP_SRV_LOGI( ... ) \
    do {                     \
    } while( false )
#define SNTP_SRV_LOGE( ... ) \
    do {                     \
    } while( false )
#define SNTP_SRV_LOGW( ... ) \
    do {                     \
    } while( false )

#endif  // ENABLE_SNTP_SRV_LOGS

namespace time_service {

using namespace std::chrono_literals;

namespace {
const char* const NtpServersList[] = { "0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org", "3.pool.ntp.org" };
}

bool SntpService::initializeImpl() {
    SNTP_SRV_LOGI( "Initializing SNTP...." );

    if( !ParentType::initializeImpl() ) {
        SNTP_SRV_LOGE( "Can't initialize ParentType" );
        return false;
    }

    if( !bus::Bus::get().join( network::wifi_srv2::responce::BUS_NAME, *this ) ) {
        SNTP_SRV_LOGE( "Can't join to %s bus namespace", network::responce::BUS_NAME );
        return false;
    }

    sntp_setoperatingmode( SNTP_OPMODE_POLL );
    for( int i = 0; i < sizeof( NtpServersList ) / sizeof( NtpServersList[0] ); ++i ) {
        sntp_setservername( i, const_cast<char*>( NtpServersList[i] ) );
    }

    if( network::wifi_srv2::isConnectedToAcessPoint() ) {
        m_sntpState = SntpState::WaitingSntpResp;
        sntp_init();
    }
    else {
        m_sntpState = SntpState::WaitingWifi;
    }

    SNTP_SRV_LOGI( "SNTP successfully initialized" );
    return true;
}

bool waitingTimeDidEstablished() {
    while( true ) {
        {
            auto inst = SntpService::get();
            if( !inst ) {
                return false;
            }

            if( inst->isTimeEstablished() ) {
                return true;
            }
        }

        core::this_thread::sleep_for( 250ms );
    }

    return false;
}

bool waitingTimeDidEstablishedFor( const std::chrono::milliseconds& delay ) {
    const auto tp = std::chrono::steady_clock::now();
    do {
        {
            auto inst = SntpService::get();
            if( !inst ) {
                return false;
            }

            if( inst->isTimeEstablished() ) {
                return true;
            }
        }

        core::this_thread::sleep_for( 100ms );
    } while( std::chrono::steady_clock::now() - tp < delay );

    return false;
}

void SntpService::onShutdownStart() {
    ParentType::onShutdownStart();

    if( SntpState::SntpTimeEstablished == m_sntpState ) {
        SNTP_SRV_LOGW( "call sntp_stop()" );
        sntp_stop();
    }
}

SntpService::ClbkRet SntpService::updateComponent( UpdContext& ctx ) {
    switch( m_sntpState ) {
    case SntpState::NotInitialized:
        return ClbkRet::Completed;
    case SntpState::WaitingWifi:
        break;
    case SntpState::WaitingSntpResp: {
        constexpr auto numberOfYearsPast1900 = 2017 - 1900;
        time_t sntpTimeNow = 0;
        time( &sntpTimeNow );
        tm sntpTimeInfo = {};
        localtime_r( &sntpTimeNow, &sntpTimeInfo );
        if( sntpTimeInfo.tm_year < numberOfYearsPast1900 ) {
            break;
        }

        m_sntpState = SntpState::SntpTimeEstablished;
        send<responce::TimeEstablished>( responce::BUS_NAME );

        char strftime_buf[64];
        strftime( strftime_buf, sizeof( strftime_buf ), "%c", &sntpTimeInfo );
        SNTP_SRV_LOGI( "Set system time from NTP success: the current local date/time is: %s", strftime_buf );
        break;
    }
    case SntpState::SntpTimeEstablished:
        break;
    }

    return ClbkRet::Continue;
}

void SntpService::visit( const bus::BusAddr from,
                         const bus::BusAddr to,
                         const network::wifi_srv2::responce::ConnectedToWifiStation& command ) {
    if( SntpState::WaitingWifi == m_sntpState ) {
        m_sntpState = SntpState::WaitingSntpResp;
        SNTP_SRV_LOGI( "call sntp_init()" );
        sntp_init();
    }
}

void SntpService::visit( const bus::BusAddr from,
                         const bus::BusAddr to,
                         const network::wifi_srv2::responce::DisconnectedFromWifiStation& command ) {
    if( SntpState::SntpTimeEstablished == m_sntpState ) {
        SNTP_SRV_LOGI( "call sntp_stop()" );
        sntp_stop();
    }

    m_sntpState = SntpState::WaitingWifi;
}

}  // namespace time_service
