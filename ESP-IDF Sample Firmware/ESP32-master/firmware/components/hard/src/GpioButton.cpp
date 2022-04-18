#include "GpioButton.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <DebugUtils.h>

#include "BusCommandGpioButton.hpp"

namespace hard {

void IRAM_ATTR GpioButton::gpio_isr_handler( GpioButton* self ) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTimerStartFromISR( self->m_debounceTimer, &xHigherPriorityTaskWoken );
}

void IRAM_ATTR GpioButton::debounce_handler( TimerHandle_t handle ) {
    GpioButton& self = *reinterpret_cast<GpioButton*>( pvTimerGetTimerID( handle ) );

    auto newButtonState = self.readButton();
    if( newButtonState == self.m_buttonState ) {
        return;
    }

    self.m_buttonState = newButtonState;
    bool startActionTimer = false;
    switch( self.m_buttonState ) {
    case ButtonState::ButtonPressed:
        self.m_clickStart = xTaskGetTickCount();
        self.m_lastClickDuration = 0;
        ++self.m_nClicks;
        break;
    case ButtonState::ButtonReleased:
        self.m_lastClickDuration = xTaskGetTickCount() - self.m_clickStart;
        self.m_clickStart = 0;
        startActionTimer = true;
        break;
    }
    if( startActionTimer ) {
        if( pdTRUE != xTimerStart( self.m_actionTimer, START_TIMER_TIMEOUT_MS / portTICK_PERIOD_MS ) ) {
            FUNC_LOGE( "Failed to start action timer" );
            self.reset();
        }
    }
}

void IRAM_ATTR GpioButton::action_handler( TimerHandle_t handle ) {
    GpioButton& self = *reinterpret_cast<GpioButton*>( pvTimerGetTimerID( handle ) );
    if( ButtonState::ButtonReleased == self.readButton() ) {
        self.setAction();
        return;
    }
}

GpioButton::GpioButton( const GpioPin gpioPin )
: m_gpioPin( gpioPin ) {
    bool result = true;
    result &= ESP_OK == gpio_set_direction( gpioPin, GPIO_MODE_INPUT );
    result &= ESP_OK == gpio_set_pull_mode( gpioPin, GPIO_PULLUP_ONLY );
    result &= ESP_OK == gpio_set_intr_type( gpioPin, GPIO_INTR_ANYEDGE );
    // install gpio isr service
    result &= ESP_OK == gpio_install_isr_service( ESP_INTR_FLAG_IRAM );
    result &= ESP_OK == gpio_isr_handler_add(
                            gpioPin, reinterpret_cast<void ( * )( void* )>( GpioButton::gpio_isr_handler ), this );
    result &= ESP_OK == gpio_intr_enable( gpioPin );

    assert( result );

    m_debounceTimer = xTimerCreate(
        "jitter", DEBOUNCE_TIME_MS / portTICK_PERIOD_MS, pdFALSE, (void*)this, &GpioButton::debounce_handler );
    assert( m_debounceTimer );
    m_actionTimer = xTimerCreate(
        "action", ACTION_TIMEOUT_MS / portTICK_PERIOD_MS, pdFALSE, (void*)this, &GpioButton::action_handler );
    assert( m_actionTimer );
    reset();
    m_buttonState = readButton();
}

GpioButton::~GpioButton() {
    gpio_intr_disable( m_gpioPin );
    gpio_isr_handler_remove( m_gpioPin );
    gpio_uninstall_isr_service();
    xTimerDelete( m_debounceTimer, 0 );
    xTimerDelete( m_actionTimer, 0 );
}

GpioButton::ButtonState IRAM_ATTR GpioButton::readButton() {
    return ( 0 == gpio_get_level( m_gpioPin ) ? GpioButton::ButtonState::ButtonPressed :
                                                GpioButton::ButtonState::ButtonReleased );
}

void IRAM_ATTR GpioButton::setAction() {
    if( !m_nClicks ) {
        reset();
        return;
    }
    if( m_nClicks == 1 ) {
        auto clickDurationMs = m_lastClickDuration * portTICK_PERIOD_MS;
        if( clickDurationMs > VERY_VERY_LONG_CLICK_MS ) {
            m_event = std::make_unique<::bus::command::GpioButtonVeryVeryLongClicked>();
        }
        else if( clickDurationMs > VERY_LONG_CLICK_MS ) {
            m_event = std::make_unique<::bus::command::GpioButtonVeryLongClicked>();
        }
        else if( clickDurationMs > LONG_CLICK_MS ) {
            m_event = std::make_unique<::bus::command::GpioButtonLongClicked>();
        }
        else if( clickDurationMs ) {
            m_event = std::make_unique<::bus::command::GpioButtonClicked>();
        }
        else {
            FUNC_LOGE( "Error processing single click" );
        }
    }
    else if( m_nClicks == 2 ) {
        m_event = std::make_unique<::bus::command::GpioButtonDoubleClicked>();
    }
    else {
        FUNC_LOGI( "Can't handle %d clicks", m_nClicks );
    }

    reset();
}

void GpioButton::reset() {
    m_nClicks = 0;
    m_clickStart = 0;
    m_lastClickDuration = 0;
    m_buttonState = ButtonState::ButtonReleased;
}

void GpioButton::update() {
    if( m_event ) {
        send(::bus::command::GPIO_BUTTON_BUS_CLIENT_NAME, std::move( m_event ) );
    }
    tryReceive( 0 );
}

}  // namespace hard
