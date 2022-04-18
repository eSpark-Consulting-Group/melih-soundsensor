#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "HouseModesManager.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubModesSwitchToDelaySet( const gason::JsonValue& json,
                                                                             common::JsonStreamGenerator& jsGen ) {
    auto errorCode = errors::NotError;
    const uint32_t delay = json( tokens::Params )( tokens::SwitchToDelay ).toInt();

    core::HouseModesManager::Result ret = core::HouseModesManager::get().setSwitchToDelay( jsGen, delay );
    bool result = ( core::HouseModesManager::Result::OK == ret );

    if( !result ) {
        switch( ret ) {
        case core::HouseModesManager::Result::NotFound:
        case core::HouseModesManager::Result::WrongArguments:
            errorCode = errors::InvalidParams;
            break;
        default:
            errorCode = errors::ApplicationError;
            break;
        }
    }

    return std::make_tuple( errorCode, StringType() );
}

}  // namespace nma