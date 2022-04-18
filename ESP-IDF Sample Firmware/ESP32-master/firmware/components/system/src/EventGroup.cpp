#include "system/EventGroup.h"

namespace sys {

FreeRtosEventGroup::FreeRtosEventGroup()
: m_eventGroup{ xEventGroupCreate() } {}

FreeRtosEventGroup::~FreeRtosEventGroup() {
    if( m_eventGroup ) {
        vEventGroupDelete( m_eventGroup );
    }
}

bool FreeRtosEventGroup::setStatusBits( EventBits_t bitsToSet ) {
    auto bits = getStatusBits() | bitsToSet;
    return bitsToSet == ( xEventGroupSetBits( m_eventGroup, bits ) & bitsToSet );
}

bool FreeRtosEventGroup::clearStatusBits( EventBits_t bitsToClear ) {
    return 0 == ( xEventGroupClearBits( m_eventGroup, bitsToClear ) & bitsToClear );
}

bool FreeRtosEventGroup::waitStatusBits( EventBits_t bits, const std::chrono::milliseconds& timeout ) {
    return bits == ( xEventGroupWaitBits( m_eventGroup,  // The event group being tested.
                                          bits,          // The bits within the event group to wait for.
                                          pdFALSE,       // BIT_0 and BIT_4 should be cleared before returning.
                                          pdTRUE,        // Don't wait for both bits, either bit will do.
                                          timeout.count() / portTICK_PERIOD_MS ) &
                     bits );
}

}  // namespace sys
