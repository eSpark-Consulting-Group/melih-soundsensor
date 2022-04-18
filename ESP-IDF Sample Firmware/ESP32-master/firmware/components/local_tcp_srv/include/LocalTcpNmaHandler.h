#pragma once

#include "NmaWebSocketHandler.h"

namespace local_tcp_srv {

class LocalTcpNmaHandler : public nma::WebSocketHandler {
public:
    using nma::WebSocketHandler::WebSocketHandler;

    ResponceType onRegisterNmaClient( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;
};

}  // namespace local_tcp_srv