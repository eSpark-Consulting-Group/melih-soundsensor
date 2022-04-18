#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "GatewayUtils.h"
#include "DeviceInfo.h"
#include "BusCommandCloudDictionaryValue.h"

#include "DebugUtils.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubItemDictionaryClear( const gason::JsonValue& json,
                                                                           common::JsonStreamGenerator& gen ) {
    int errorCode = errors::NotError;
    StringType errorMessage;

    bool isSuccess = gen.writeEmptyObject() && gen.writeItemDelimiter();

    auto params = json( tokens::Params );
    auto itemId = params( tokens::SubId ).toString();

    auto itemInfo = gateways::ItemInfo::create( itemId );
    if( !itemInfo ) {
        FUNC_LOGE( "Wrong itemId %s", itemId );
        isSuccess = false;
        errorCode = errors::InvalidParams;
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
                               pluginName, itemId, std::string{}, ::bus::command::CloudDictionaryValue::Clear ) );
        }
    }
    else {
        if( errorCode == errors::NotError ) {
            errorCode = errors::ApplicationError;
        }
        errorMessage = commands::HubItemDictionaryClear;
    }

    return std::make_tuple( errorCode, std::move( errorMessage ) );
}

}  // namespace nma
