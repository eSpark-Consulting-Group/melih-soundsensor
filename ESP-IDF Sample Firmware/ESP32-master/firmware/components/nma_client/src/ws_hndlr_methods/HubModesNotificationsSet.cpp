#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "HouseModesManager.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubModesNotificationsSet( const gason::JsonValue& json,
                                                                             common::JsonStreamGenerator& jsGen ) {
    auto errorCode = errors::NotError;
    jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();
    auto params = json( tokens::Params );
    bool result = (bool)params;

    uint8_t mode = core::HouseModesManager::PRESET_ID_INVALID;
    const char* modeStr = params( tokens::ModeId ).toString();
    result = result && modeStr && 1 == sscanf( modeStr, "%hhu", &mode );

    result = result && params( tokens::All ).isBoolean();
    bool all = result && params( tokens::All ).toBool();
    std::vector<std::string> users;
    if( result && !all && params( tokens::UserIds ).isArray() ) {
        for( const auto user : params( tokens::UserIds ) ) {
            const char* user_str = user.value.toString();
            if( user_str ) {
                users.emplace_back( user_str );
            }
        }
    }

    core::HouseModesManager::Result ret = core::HouseModesManager::Result::SystemError;
    if( result ) {
        ret = core::HouseModesManager::get().setNotifications( jsGen, mode, all, std::move( users ) );
        switch( ret ) {
        case core::HouseModesManager::Result::OK:
            errorCode = errors::NotError;
            break;
        case core::HouseModesManager::Result::NotFound:
        case core::HouseModesManager::Result::WrongArguments:
            errorCode = errors::InvalidParams;
            break;
        default:
            errorCode = errors::ApplicationError;
            break;
        }
    }
    else {
        errorCode = errors::InvalidParams;
    }

    return std::make_tuple( errorCode, StringType() );
}

}  // namespace nma