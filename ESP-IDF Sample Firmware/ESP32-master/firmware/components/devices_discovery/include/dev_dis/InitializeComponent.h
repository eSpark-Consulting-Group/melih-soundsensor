#pragma once

#include "sdkconfig.h"

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

namespace dev_dis {

bool initializeComponent();

}  // namespace dev_dis

#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
