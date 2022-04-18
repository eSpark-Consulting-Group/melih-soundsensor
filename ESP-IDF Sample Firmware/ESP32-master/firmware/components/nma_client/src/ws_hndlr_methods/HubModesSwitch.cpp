#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "HouseModesManager.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubModesSwitch( const gason::JsonValue& json,
                                                                   common::JsonStreamGenerator& jsGen ) {
    auto errorCode = errors::NotError;
    core::HouseModesManager::Result ret;

    const char* modeStr = json( tokens::Params )( tokens::ModeId ).toString();
    if( modeStr ) {
        uint8_t mode = core::HouseModesManager::PRESET_ID_INVALID;
        sscanf( modeStr, "%hhu", &mode );
        ret = core::HouseModesManager::get().switchPreset( jsGen, mode );
    }
    else {
        const auto name = json( tokens::Params )( tokens::Name ).toString();
        if( name ) {
            ret = core::HouseModesManager::get().switchPreset( jsGen, name );
        }
        else {
            ret = core::HouseModesManager::Result::WrongArguments;
        }
    }

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