#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "HouseModesManager.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubModesAlarmDelaySet( const gason::JsonValue& json,
                                                                          common::JsonStreamGenerator& jsGen ) {
    bool isSuccess;
    const uint32_t delay = json( tokens::Params )( tokens::AlarmDelay ).toInt( &isSuccess );
    if( !isSuccess ) {
        return std::make_tuple( errors::InvalidParams, StringType() );
    }

    auto ret = core::HouseModesManager::get().setAlarmDelay( jsGen, delay );
    if( core::HouseModesManager::Result::OK != ret ) {
        switch( ret ) {
        case core::HouseModesManager::Result::NotFound:
        case core::HouseModesManager::Result::WrongArguments:
            return std::make_tuple( errors::InvalidParams, StringType() );
        default:
            return std::make_tuple( errors::ApplicationError, StringType() );
        }
    }

    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace nma