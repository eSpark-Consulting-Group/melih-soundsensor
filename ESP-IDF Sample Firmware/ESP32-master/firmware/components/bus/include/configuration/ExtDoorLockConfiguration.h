#pragma once

#include <cstdint>

namespace configuration {

struct ExtDoorLockConfiguration {
    uint16_t m_autoRelockTimeout;
    uint16_t m_holdAndReleaseTimeout;
    bool m_twistAssistEnabled;
    bool m_blockToBlockEnabled;
};

}  // namespace configuration
