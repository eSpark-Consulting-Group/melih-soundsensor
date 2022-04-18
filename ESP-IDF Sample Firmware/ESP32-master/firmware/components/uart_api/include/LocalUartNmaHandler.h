#pragma once

#include "NmaWebSocketHandler.h"

namespace local_uart_srv {

class LocalUartNmaHandler : public nma::WebSocketHandler {
public:
    using nma::WebSocketHandler::WebSocketHandler;
};

}  // namespace local_uart_srv