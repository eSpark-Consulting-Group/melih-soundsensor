#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "values/UserCodeValue.h"
#include "DeviceInfo.h"
#include "Utils.h"
#include "BusCommandCloudDictionaryValue.h"
#include "IValueCast.h"
#include "GatewayUtils.h"

#include "DebugUtils.h"

extern "C" {
#include "re.h"
}

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubItemDictionaryValueSet( const gason::JsonValue& json,
                                                                              common::JsonStreamGenerator& jsGen ) {
    bool isSuccess = jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();

    auto params = json( tokens::Params );
    const char* itemId = params( tokens::SubId ).toString();
    const char* key = params( tokens::Key ).toString();
    const char* typeValue = params( tokens::Element )( tokens::Type ).toString();

    auto jsValue = params( tokens::Element )( tokens::Value );
    auto iValue = common::makeIval( jsValue );

    int errorCode = errors::NotError;
    StringType errorMessage;

    auto itemInfo = gateways::ItemInfo::create( itemId );
    if( !itemInfo ) {
        FUNC_LOGE( "Wrong itemId %s", itemId );
        isSuccess = false;
        errorCode = errors::InvalidParams;
    }
    else {
        if( !strcasecmp( typeValue, values::Usercode ) ) {
            // check for valid iValue
            while( 1 ) {
                auto tblVal = common::iValueCast<common::UserCodeValue>( iValue->clone() );
                if( !tblVal ) {
                    FUNC_LOGE( "Wrong field name for user code" );
                    errorCode = errors::InvalidParams;
                    isSuccess = false;
                    break;
                }
                auto code = tblVal->getCode();

                gateways::ItemInfo& refItemInfo = *( itemInfo.get() );

                std::string restriction = "^\\d{4,10}";
                if( !refItemInfo.getUserCodeRestriction().empty() )
                    restriction = refItemInfo.getUserCodeRestriction();

                int match_length;
                int match_idx = re_match( restriction.c_str(), code.c_str(), &match_length );
                if( match_idx == -1 ) {
                    FUNC_LOGE( "Code \"%s\" doesn`t match to restriction \"%s\"", code.c_str(), restriction.c_str() );
                    errorCode = errors::InvalidParams;
                    isSuccess = false;
                    break;
                }
                break;
            }
        }
    }
    if( isSuccess && iValue ) {
        auto pluginName = ::gateways::utils::getPluginNameByItemId( itemId );
        if( pluginName.empty() ) {
            FUNC_LOGE( "Can't get a plugin's name by an item's id" );
            errorCode = errors::ApplicationError;
            isSuccess = false;
        }
        else {
            m_client.send(
                "debug",
                std::make_unique<::bus::command::CloudDictionaryValue>( pluginName,
                                                                        itemId,
                                                                        std::move( iValue ),
                                                                        typeValue,
                                                                        key,
                                                                        ::bus::command::CloudDictionaryValue::Set ) );
        }
    }

    if( !isSuccess ) {
        if( errorCode == errors::NotError )
            errorCode = errors::ApplicationError;
        errorMessage = commands::HubItemDictionaryValueSet;
    }

    return std::make_tuple( errorCode, std::move( errorMessage ) );
}

}  // namespace nma
