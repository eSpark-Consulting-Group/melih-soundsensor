#pragma once

#include "NmaWebSocketHandler.h"

namespace ucloud_client {

class UNmaJsonHandler : public nma::WebSocketHandler {
public:
    using nma::WebSocketHandler::WebSocketHandler;

public:  // nma::WebSocketHandler
    ResponceType onRegistered( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) override;
};

}  // namespace ucloud_client