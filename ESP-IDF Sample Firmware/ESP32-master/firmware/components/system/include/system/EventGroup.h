#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

#include <chrono>

namespace sys {

class FreeRtosEventGroup {
public:
    FreeRtosEventGroup();

    ~FreeRtosEventGroup();

    inline EventBits_t getStatusBits() const {
        return xEventGroupGetBits( m_eventGroup );
    }

    bool waitStatusBits( EventBits_t bits, const std::chrono::milliseconds& timeout );

    bool setStatusBits( EventBits_t bits );

    bool clearStatusBits( EventBits_t bits );

private:
    EventGroupHandle_t m_eventGroup = nullptr;
};  // class FreeRtosEventGroup

}  // namespace sys
