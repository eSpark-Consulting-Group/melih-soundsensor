#include <LedDevice.h>

#include "soc/gpio_periph.h"

#include "DebugUtils.h"

#define TIMER_DIVIDER 16
#define TIMER_SCALE ( TIMER_BASE_CLK / TIMER_DIVIDER / 1000 )  // convert counter value to seconds

namespace hard {

namespace {

constexpr uint32_t activeLevel = CONFIG_LED_ACTIVE_LEVEL;

constexpr auto TimerGroup = TIMER_GROUP_1;

bool IRAM_ATTR timer_group0_isr( void* context ) {
    auto* _led = static_cast<LedDevice*>( context );

    // Retrieve the interrupt status from the timer that reported the interrupt
    uint32_t intr_status = timer_group_get_intr_status_in_isr( TimerGroup );

    // Clear the interrupt
    if( intr_status & BIT( _led->timerId() ) ) {
        _led->timerTick();
    }

    return false;
}

}  // namespace

LedDevice::LedDevice( const int ledPin )
: mLedPin( gpio_num_t( ledPin ) ) {}

LedDevice::~LedDevice() {
    if( ESP_OK != timer_pause( TimerGroup, mTimerId ) ) {
        FUNC_LOGE( "timer_pause failed" );
    }

    if( ESP_OK != timer_isr_callback_remove( TimerGroup, mTimerId ) ) {
        FUNC_LOGE( "timer_isr_callback_remove failed" );
    }

    if( ESP_OK != timer_disable_intr( TimerGroup, mTimerId ) ) {
        FUNC_LOGE( "timer_disable_intr failed" );
    }

    if( ESP_OK != timer_deinit( TimerGroup, mTimerId ) ) {
        FUNC_LOGE( "timer_deinit failed" );
    }
}

void LedDevice::setBlink( uint16_t number, uint32_t intervalMsec ) {
    m_ticsCount = number * 2;
    m_onIntervalMsec = m_offIntervalMsec = intervalMsec;
    startTimer();
}

void LedDevice::setBlink( uint16_t number, uint32_t onIntervalMsec, uint32_t offIntervalMsec ) {
    m_ticsCount = number * 2;
    m_onIntervalMsec = onIntervalMsec;
    m_offIntervalMsec = offIntervalMsec;
    startTimer();
}

void IRAM_ATTR LedDevice::setLedState( const LedState newState ) {
    auto level = On == newState ? activeLevel : !activeLevel;
    if( level ) {
        if( mLedPin < 32 ) {
            GPIO.out_w1ts = ( 1 << mLedPin );
        }
        else {
            GPIO.out1_w1ts.data = ( 1 << ( mLedPin - 32 ) );
        }
    }
    else {
        if( mLedPin < 32 ) {
            GPIO.out_w1tc = ( 1 << mLedPin );
        }
        else {
            GPIO.out1_w1tc.data = ( 1 << ( mLedPin - 32 ) );
        }
    }

    mLedState = newState;
}

void IRAM_ATTR LedDevice::timerTick() {
    stopTimer();
    if( --m_ticsCount != 0 ) {
        invertLedState();
        startTimer();
    }
    else {
        setLedState( Off );
    }
}

void IRAM_ATTR LedDevice::startTimer() {
    timer_set_counter_value( TimerGroup, mTimerId, 0x00000000ULL );
    timer_set_alarm_value(
        TimerGroup, mTimerId, ( mLedState == On ? m_onIntervalMsec : m_offIntervalMsec ) * TIMER_SCALE );
    timer_start( TimerGroup, mTimerId );
    mIsTimerOn = true;
}

void IRAM_ATTR LedDevice::stopTimer() {
    if( ESP_OK != timer_pause( TimerGroup, mTimerId ) ) {
        return;
    }
    mIsTimerOn = false;
}

bool LedDevice::initializeImpl() {
    gpio_pad_select_gpio( mLedPin );
    if( ESP_OK != gpio_set_direction( mLedPin, GPIO_MODE_OUTPUT ) ) {
        return false;
    }
    setLedState( Off );

    timer_config_t config = {};
    config.divider = TIMER_DIVIDER;
    config.counter_dir = TIMER_COUNT_UP;
    config.counter_en = TIMER_PAUSE;
    config.alarm_en = TIMER_ALARM_EN;
    config.intr_type = TIMER_INTR_LEVEL;
    config.auto_reload = TIMER_AUTORELOAD_EN;

    if( ESP_OK != timer_init( TimerGroup, mTimerId, &config ) ) {
        FUNC_LOGE( "timer_init failed" );
        return false;
    }

    if( ESP_OK != timer_enable_intr( TimerGroup, mTimerId ) ) {
        FUNC_LOGE( "timer_enable_intr failed" );
        return false;
    }

    if( ESP_OK != timer_isr_callback_add( TimerGroup, mTimerId, timer_group0_isr, this, ESP_INTR_FLAG_LOWMED ) ) {
        FUNC_LOGE( "timer_enable_intr failed" );
        return false;
    }

    return true;
}

void IRAM_ATTR LedDevice::ledOn() {
    stopTimer();
    setLedState( On );
}

void IRAM_ATTR LedDevice::ledOff() {
    stopTimer();
    setLedState( Off );
};

}  // namespace hard
