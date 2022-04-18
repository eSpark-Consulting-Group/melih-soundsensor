#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "HouseModesManager.h"
#include "DebugUtils.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubModesDisarmedDefaultSet( const gason::JsonValue& json,
                                                                               common::JsonStreamGenerator& jsGen ) {
    auto errorCode = errors::NotError;
    jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();
    uint8_t mode = core::HouseModesManager::PRESET_ID_INVALID;
    const char* modeStr = json( tokens::Params )( tokens::ModeId ).toString();
    if( modeStr ) {
        sscanf( modeStr, "%hhu", &mode );
    }
    bool def = json( tokens::Params )( tokens::DisarmedDefault ).toBool();
    bool result = json( tokens::Params )( tokens::DisarmedDefault ).isBoolean();
    core::HouseModesManager::Result ret = core::HouseModesManager::get().setDisarmedDefault( jsGen, mode, def );
    result = result && ( core::HouseModesManager::Result::OK == ret );
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
    // TODO: temporary solution for EATT-1218 and EATT-1223.
    // Set user notifications to default on "hub.modes.disarmed_default.set" command
    if( def && core::HouseModesManager::Result::OK != core::HouseModesManager::get().setDefaultNotifications( mode ) ) {
        FUNC_LOGE( "Failed to reset user notifications to default" );
    }

    return std::make_tuple( errorCode, StringType() );
}

}  // namespace nma