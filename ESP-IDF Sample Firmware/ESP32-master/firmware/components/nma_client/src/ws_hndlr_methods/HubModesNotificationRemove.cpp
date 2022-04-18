#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "HouseModesManager.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubModesNotificationRemove( const gason::JsonValue& json,
                                                                               common::JsonStreamGenerator& jsGen ) {
    auto errorCode = errors::NotError;
    jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();
    uint8_t mode = core::HouseModesManager::PRESET_ID_INVALID;
    const char* modeStr = json( tokens::Params )( tokens::ModeId ).toString();
    if( modeStr ) {
        sscanf( modeStr, "%hhu", &mode );
    }
    const char* user = json( tokens::Params )( tokens::UserId ).toString();
    bool result = true;
    core::HouseModesManager::Result ret = core::HouseModesManager::get().removeNotification( jsGen, mode, user );
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

    return std::make_tuple( errorCode, StringType() );
}

}  // namespace nma