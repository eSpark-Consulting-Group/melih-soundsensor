#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "GatewayUtils.h"
#include "DeviceInfo.h"
#include "BusCommandCloudDictionaryValue.h"

extern "C" {
#include "re.h"
}

#include "DebugUtils.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubItemDictionaryValueRemove( const gason::JsonValue& json,
                                                                                 common::JsonStreamGenerator& gen ) {
    int errorCode = errors::NotError;
    StringType errorMessage;

    bool isSuccess = gen.writeEmptyObject() && gen.writeItemDelimiter();

    auto params = json( tokens::Params );
    auto itemId = params( tokens::SubId ).toString();
    auto key = params( tokens::Key ).toString();

    auto itemInfo = gateways::ItemInfo::create( itemId );
    if( !itemInfo ) {
        FUNC_LOGE( "Wrong itemId %s", itemId );
        isSuccess = false;
        errorCode = errors::InvalidParams;
    }
    else {
        int match_length;
        int match_idx = re_match( "^\\d+", key, &match_length );
        if( match_idx == -1 ) {
            FUNC_LOGE( "Incorrect field key" );
            isSuccess = false;
            errorCode = errors::InvalidParams;
        }
    }

    if( isSuccess ) {
        auto pluginName = ::gateways::utils::getPluginNameByItemId( itemId );
        if( pluginName.empty() ) {
            FUNC_LOGE( "Can't get a plugin's name by an item's id" );
            errorCode = errors::ApplicationError;
            isSuccess = false;
        }
        else {
            m_client.send( "debug",
                           std::make_unique<::bus::command::CloudDictionaryValue>(
                               pluginName, itemId, key, ::bus::command::CloudDictionaryValue::Remove ) );
        }
    }
    else {
        if( errorCode == errors::NotError ) {
            errorCode = errors::ApplicationError;
        }
        errorMessage = commands::HubItemDictionaryValueRemove;
    }

    return std::make_tuple( errorCode, std::move( errorMessage ) );
}

}  // namespace nma
