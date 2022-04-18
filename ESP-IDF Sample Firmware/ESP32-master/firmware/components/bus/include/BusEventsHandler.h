#pragma once

#include "BusAddr.h"

#include <string>

namespace bus {

struct EventsHandler {
    BusAddr m_addr{ BusAddrInvalid };
    std::string m_handlerName;
};

}  // namespace bus
