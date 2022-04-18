#include "LedControl.h"

#include "lua/responce/HubSendBroadcast.h"
#include "CommonResources.h"

#include "DebugUtils.h"

#define LED_CTR_LOG_TAG "[LedControl]"
#define LED_CTR_LOGE( _FMT, ... ) ESP_LOGE( LED_CTR_LOG_TAG, _FMT, ##__VA_ARGS__ )

//#define ALL_LED_CONTROL_LOGS
#ifdef ALL_LED_CONTROL_LOGS

#define LED_CTR_LOGW( _FMT, ... ) ESP_LOGW( LED_CTR_LOG_TAG, _FMT, ##__VA_ARGS__ )
#define LED_CTR_LOGI( _FMT, ... ) ESP_LOGI( LED_CTR_LOG_TAG, _FMT, ##__VA_ARGS__ )

#else  // ALL_LED_CONTROL_LOGS

#define LED_CTR_LOGW( ... ) \
    do {                    \
    } while( false )
#define LED_CTR_LOGI( ... ) \
    do {                    \
    } while( false )

#endif  // ALL_LED_CONTROL_LOGS

namespace hard {

namespace {

constexpr auto SoftResetBlinkPeriod = 100;
constexpr auto WifiResetBlinkPeriod = 300;
constexpr auto WifiDisconnBlinkPeriod = 1000;
constexpr auto ServerDisconnBlinkPeriod = 3000;
constexpr auto ExcludeStartedBlinkPeriod = 100;
constexpr auto IncludeStartedBlinkPeriod = 100;
constexpr auto SendNifBlinkPeriod = 200;
constexpr auto IndicatorIdentifyPeriodOn = 600;
constexpr auto IndicatorIdentifyPeriodOff = 200;

using namespace common::resources;

}  // namespace

bool LedControl::initializeImpl() {
    LED_CTR_LOGI( "initializeImpl()" );

    if( !ParentType::initializeImpl() ) {
        LED_CTR_LOGE( "Can't initialize parent type!" );
        return false;
    }

    if( !bus::Bus::get().join( lua::responce::BUS_NAME, *this ) ) {
        LED_CTR_LOGE( "Can't join to \"nma\" namespace" );
        return false;
    }

    if( !bus::Bus::get().join( network::wifi_srv2::responce::BUS_NAME, *this ) ) {
        LED_CTR_LOGE( "Can't join to %s bus namespace", network::wifi_srv2::responce::BUS_NAME );
        return false;
    }

    if( !bus::Bus::get().join( cloud_client::responces::BUS_NAME, *this ) ) {
        LED_CTR_LOGE( "Can't join to bus namespace: %s", cloud_client::responces::BUS_NAME );
        return false;
    }

    auto led = std::make_unique<LedDevice>( CONFIG_IO_LED );
    if( led && led->initialize() ) {
        m_led = std::move( led );
    }
    else {
        return false;
    }

    LED_CTR_LOGI( "Successfully initialized" );
    return true;
}

void LedControl::setIsWifiConfigExist( const bool isWifiConfExist ) {
    setState( isWifiConfExist ? State::NON_WIFI_LD : State::resetWiFi_LD );
}

void LedControl::visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::HubSendBroadcast& cmd ) {
    if( broadcasts::events::ExcludeInvoked == cmd.m_event ) {
        processExcludeInvoked();
    }
    else if( broadcasts::events::ExcludeStarted == cmd.m_event ) {
        processExcludeStarted();
    }
    else if( broadcasts::events::ExcludeFinished == cmd.m_event ||
             broadcasts::events::ExcludeFinishedError == cmd.m_event ||
             broadcasts::events::ExcludeTimeout == cmd.m_event ) {
        processExcludeStopped();
    }

    else if( broadcasts::events::IncludeInvoked == cmd.m_event ) {
        processIncludeInvoked();
    }
    else if( broadcasts::events::IncludeStarted == cmd.m_event ) {
        processIncludeStarted();
    }
    else if( broadcasts::events::IncludeFinished == cmd.m_event ||
             broadcasts::events::IncludeFinishedError == cmd.m_event ||
             broadcasts::events::IncludeTimeout == cmd.m_event ) {
        processIncludeStopped();
    }
}

void LedControl::visit( const bus::BusAddr from,
                        const bus::BusAddr to,
                        const cloud_client::responces::ConnectedToCloud& command ) {
    processConnectedToCloud();
}

void LedControl::visit( const bus::BusAddr from,
                        const bus::BusAddr to,
                        const cloud_client::responces::DisconnectedFromCloud& command ) {
    processDisconnectedFromCloud();
}

void LedControl::visit( const bus::BusAddr from,
                        const bus::BusAddr to,
                        const network::wifi_srv2::responce::ConnectedToWifiStation& command ) {
    processConnectedToWifi();
}

void LedControl::visit( const bus::BusAddr from,
                        const bus::BusAddr to,
                        const network::wifi_srv2::responce::DisconnectedFromWifiStation& command ) {
    processLostWifi();
}

void LedControl::setState( const State newState ) {
    LED_CTR_LOGI( "setState(currState=%d, newState=%d)", int( m_currState ), int( newState ) );
    std::lock_guard<decltype( m_mutex )> guard( m_mutex );

    switch( newState ) {
    case State::IDLE:
        LED_CTR_LOGI( "     case State::IDLE:" );
#ifdef CONFIG_BOARD_TYPE_PLUG_HUB
        if( m_plugSwitchisOn ) {
            led().ledOn();
        }
        else {
            led().ledOff();
        }
#else   // CONFIG_BOARD_TYPE_PLUG_HUB
        led().ledOff();
#endif  // CONFIG_BOARD_TYPE_PLUG_HUB
        break;
    case State::SOFT_RESET_LD:
        LED_CTR_LOGI( "     case State::SOFT_RESET_LD" );
        led().setBlink( UINT16_MAX, SoftResetBlinkPeriod );
        break;
    case State::HARD_RESET_LD:
        LED_CTR_LOGI( "     case State::HARD_RESET_LD" );
        led().ledOn();
        break;
    case State::FactoryDefaults_LD:
        LED_CTR_LOGI( "     case State::HARD_RESET_LD" );
        led().ledOff();
        break;
    case State::IncludeInvoked_LD:
        LED_CTR_LOGI( "     case State::IncludeInvoked_LD" );
        led().ledOn();
        break;
    case State::INCLUDE_LD:
        LED_CTR_LOGI( "     case State::INCLUDE_LD" );
        led().setBlink( UINT16_MAX, IncludeStartedBlinkPeriod );
        break;
    case State::STOP_INCLUDE_LD:
        LED_CTR_LOGI( "     case State::STOP_INCLUDE_LD" );
        // Nothing to do
        break;
    case State::ExcludeInvoked_LD:
        LED_CTR_LOGI( "     case State::ExcludeInvoked_LD" );
        led().ledOn();
        break;
    case State::EXCLUDE_LD:
        LED_CTR_LOGI( "     case State::EXCLUDE_LD" );
        led().setBlink( UINT16_MAX, ExcludeStartedBlinkPeriod );
        break;
    case State::STOP_EXCLUDE_LD:
        LED_CTR_LOGI( "     case State::STOP_EXCLUDE_LD" );
        // Nothing to do
        break;
    case State::resetWiFi_LD:
        LED_CTR_LOGI( "     case State::resetWiFi_LD" );
        led().setBlink( UINT16_MAX, WifiResetBlinkPeriod );
        break;
    case State::NON_WIFI_LD:
        LED_CTR_LOGI( "     case State::NON_WIFI_LD" );
        led().setBlink( UINT16_MAX, WifiDisconnBlinkPeriod );
        break;
    case State::ServerDisconn_LD:
        LED_CTR_LOGI( "     case State::ServerDisconn_LD" );
        led().setBlink( UINT16_MAX, ServerDisconnBlinkPeriod );
        break;
    }

    m_currState = newState;
}

void LedControl::processStartHardReset() {
    LED_CTR_LOGI( "processStartHardReset(currState=%d)", int( currState() ) );

    switch( currState() ) {
    case State::HARD_RESET_LD:
        break;
    default:
        setState( State::HARD_RESET_LD );
    }
}

void LedControl::processStartSoftReset() {
    LED_CTR_LOGI( "processStartSoftReset(currState=%d)", int( currState() ) );

    switch( currState() ) {
    case State::SOFT_RESET_LD:
        break;
    default:
        setState( State::SOFT_RESET_LD );
    }
}

void LedControl::processResetToFactoryDefaults() {
    LED_CTR_LOGI( "processResetToFactoryDefaults(currState=%d)", int( currState() ) );

    switch( currState() ) {
    case State::FactoryDefaults_LD:
        break;
    default:
        setState( State::FactoryDefaults_LD );
    }
}

void LedControl::processResetWifi() {
    LED_CTR_LOGI( "processResetWifi(currState=%d)", int( currState() ) );

    switch( currState() ) {
    case State::resetWiFi_LD:
        break;
    default:
        setState( State::resetWiFi_LD );
    }
}

void LedControl::processLostWifi() {
    LED_CTR_LOGI( "processLostWifi(currState=%d)", int( currState() ) );

    switch( currState() ) {
    case State::HARD_RESET_LD:
    case State::SOFT_RESET_LD:
    case State::resetWiFi_LD:
        break;
    case State::NON_WIFI_LD:
        break;
    default:
        setState( State::NON_WIFI_LD );
    }
}

void LedControl::processConnectedToWifi() {
    LED_CTR_LOGI( "processConnectedToWifi(currState=%d)", int( currState() ) );

    switch( currState() ) {
    case State::ServerDisconn_LD:
        break;
    case State::HARD_RESET_LD:
    case State::SOFT_RESET_LD:
    case State::resetWiFi_LD:
    case State::NON_WIFI_LD:
        setState( State::ServerDisconn_LD );
    default:
        break;
    }
}

void LedControl::processDisconnectedFromCloud() {
    LED_CTR_LOGI( "processDisconnectedFromCloud(currState=%d)", int( currState() ) );

    switch( currState() ) {
    case State::ServerDisconn_LD:
        break;
    case State::IDLE:
        setState( State::ServerDisconn_LD );
    default:
        break;
    }
}

void LedControl::processConnectedToCloud() {
    LED_CTR_LOGI( "processConnectedToCloud(currState=%d)", int( currState() ) );

    switch( currState() ) {
    case State::IDLE:
        break;
    default:
        setState( State::IDLE );
    }
}

void LedControl::processSendNif() {
    LED_CTR_LOGI( "processSendNif(currState=%d)", int( currState() ) );

    switch( currState() ) {
    case State::IDLE:
        led().setBlink( 1, SendNifBlinkPeriod );
        break;
    default:
        break;
    }
}

void LedControl::processIndicatorIdentify() {
    LED_CTR_LOGI( "processIndicatorIdentify(currState=%d)", int( currState() ) );

    std::lock_guard<decltype( m_mutex )> guard( m_mutex );

    if( State::IDLE == currState() ) {
        led().setBlink( 3, IndicatorIdentifyPeriodOn, IndicatorIdentifyPeriodOff );
    }
}

void LedControl::processExcludeInvoked() {
    LED_CTR_LOGI( "processExcludeInvoked(currState=%d)", int( currState() ) );

    switch( currState() ) {
    case State::ExcludeInvoked_LD:
        break;
    case State::IDLE:
        setState( State::ExcludeInvoked_LD );
        break;
    default:
        break;
    }
}

void LedControl::processExcludeStarted() {
    LED_CTR_LOGI( "processExcludeStarted(currState=%d)", int( currState() ) );

    switch( currState() ) {
    case State::EXCLUDE_LD:
        break;
    case State::ExcludeInvoked_LD:
        setState( State::EXCLUDE_LD );
        break;
    default:
        break;
    }
}

void LedControl::processExcludeStopped() {
    LED_CTR_LOGI( "processExcludeStopped(currState=%d)", int( currState() ) );

    switch( currState() ) {
    case State::IDLE:
        break;
    case State::EXCLUDE_LD:
    case State::ExcludeInvoked_LD:
        setState( State::IDLE );
        break;
    default:
        break;
    }
}

void LedControl::processIncludeInvoked() {
    LED_CTR_LOGI( "processIncludeInvoked(currState=%d)", int( currState() ) );

    switch( currState() ) {
    case State::IncludeInvoked_LD:
        break;
    case State::IDLE:
        setState( State::IncludeInvoked_LD );
        break;
    default:
        break;
    }
}

void LedControl::processIncludeStarted() {
    LED_CTR_LOGI( "processIncludeStarted(currState=%d)", int( currState() ) );

    switch( currState() ) {
    case State::INCLUDE_LD:
        break;
    case State::IncludeInvoked_LD:
        setState( State::INCLUDE_LD );
        break;
    default:
        break;
    }
}

void LedControl::processIncludeStopped() {
    LED_CTR_LOGI( "processIncludeStopped(currState=%d)", int( currState() ) );

    switch( currState() ) {
    case State::IDLE:
        break;
    case State::IncludeInvoked_LD:
    case State::INCLUDE_LD:
        setState( State::IDLE );
        break;
    default:
        break;
    }
}

#ifdef CONFIG_BOARD_TYPE_PLUG_HUB
void LedControl::processPlugOn( const bool isOn ) {
    LED_CTR_LOGI( "processPlugOn(currState=%d, isOn=%d)", int( currState() ), int( isOn ) );

    std::lock_guard<decltype( m_mutex )> guard( m_mutex );

    switch( currState() ) {
    case State::IDLE:
        if( isOn ) {
            led().ledOn();
        }
        else {
            led().ledOff();
        }
        break;
    default:
        break;
    }

    m_plugSwitchisOn = isOn;
}
#endif  // CONFIG_BOARD_TYPE_PLUG_HUB;

}  // namespace hard
