#pragma once

#include <cstdint>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace hard {

class ResetZW {
public:
    static ResetZW& get();
    bool initialize();
    void resetPinAssert();
    void resetPinRelease();

private:
    ResetZW() = default;
};

}  // namespace hard
