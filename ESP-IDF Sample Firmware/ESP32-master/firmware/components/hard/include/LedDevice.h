#pragma once

#include "IInitialize.h"

#include <cstdint>
#include <driver/timer.h>
#include <driver/gpio.h>

namespace hard {

class LedDevice : public common::IInitialize {
public:
    static constexpr auto defaultPeriod_ms = 200;

    enum LedState { On, Off };

    LedDevice( const int _led_pin );

    ~LedDevice();

    /**
     * Blink with same ON and OFF intervals. Starting with OFF phase.
     */
    void setBlink( uint16_t number, uint32_t intervalMsec = defaultPeriod_ms );

    /**
     * Blink with given ON and OFF intervals. Starting with OFF phase.
     */
    void setBlink( uint16_t number, uint32_t onIntervalMsec, uint32_t offIntervalMsec );

    inline LedState ledState() const {
        return mLedState;
    }

    void IRAM_ATTR setLedState( const LedState newState );

    inline void IRAM_ATTR invertLedState() {
        setLedState( Off == ledState() ? On : Off );
    }

    void IRAM_ATTR ledOn();

    void IRAM_ATTR ledOff();

    inline bool isBlinking() const {
        return mIsTimerOn;
    }

    void IRAM_ATTR timerTick();

    inline timer_idx_t IRAM_ATTR timerId() const {
        return mTimerId;
    }

private:
    void IRAM_ATTR startTimer();
    void IRAM_ATTR stopTimer();

    bool initializeImpl() override;

private:
    uint32_t m_ticsCount = 0;
    uint32_t m_onIntervalMsec = 0;
    uint32_t m_offIntervalMsec = 0;
    LedState mLedState = Off;
    timer_idx_t mTimerId = TIMER_0;
    bool mIsTimerOn = true;
    gpio_num_t mLedPin;
};

}  // namespace hard
