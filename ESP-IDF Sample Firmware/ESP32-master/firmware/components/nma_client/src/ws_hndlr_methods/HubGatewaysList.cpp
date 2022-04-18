#include "values/ArrayValue.h"
#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "IGatewayUserDataStorage.h"
#include "GatewaySettingsInfo.hpp"
#include "SettingStorage.h"

#include "IGateway.h"

namespace nma {

namespace tokens = ::common::resources::tokens;
namespace errors = ::common::resources::errors;

WebSocketHandler::ResponceType WebSocketHandler::onHubGatewaysList( const gason::JsonValue& json,
                                                                    common::JsonStreamGenerator& gen ) {
    auto gatewayList = std::make_unique<::common::ArrayValue>();
    gateways::enumerateGateways( [this, &gatewayList]( gateways::GatewayID gatewayId, gateways::IGateway* ) {
        if( auto gatewayUserData = m_gatewayUserDataStorage.getUserData( gatewayId ) ) {
            gatewayUserData->remove( tokens::Id );
            gatewayUserData->insert( tokens::SubId, ::common::IValue::create(::gateways::toString( gatewayId ) ) );

            int settingsNumber = 0;
            {
                bool isSuccess = true;
                const auto gateway = ::gateways::toString( gatewayId );
                auto clbk =
                    [&isSuccess, &settingsNumber, &gateway]( const gateways::GatewaySettingsInfo& settingInfo ) {
                        if( gateway != settingInfo.getItemId() ) {
                            return true;
                        }

                        settingsNumber++;

                        return true;
                    };

                ::storage::GatewaySettings::get().enumerate( clbk );
            }

            gatewayUserData->insert(::common::resources::tokens::Settings,
                                    std::make_unique<::common::IntValue>( settingsNumber ) );
            gatewayList->push_back( std::move( gatewayUserData ) );
        }
        return true;
    } );
    bool isSuccess = gen.writeOpeningObject();

    isSuccess = isSuccess && gen.writeProperty( tokens::Gateways, gatewayList.get() );
    isSuccess = isSuccess && gen.writeClosingSymbol();
    if( isSuccess ) {
        return std::make_tuple( errors::NotError, StringType() );
    }
    else {
        return std::make_tuple( errors::ApplicationError, StringType() );
    }
}

}  // namespace nma
