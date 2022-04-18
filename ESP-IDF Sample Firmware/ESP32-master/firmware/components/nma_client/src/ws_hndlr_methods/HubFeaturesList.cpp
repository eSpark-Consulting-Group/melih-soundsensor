#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "FeatureFlagController.h"

using namespace common::resources;

namespace nma {

WebSocketHandler::ResponceType WebSocketHandler::onHubFeaturesList( const gason::JsonValue& json,
                                                                    common::JsonStreamGenerator& gen ) {
    auto featureFlagController = core::features::FeatureFlagController::get();
    if( !featureFlagController ) {
        gen.writeEmptyObject() && gen.writeItemDelimiter();
        return std::make_tuple( errors::ApplicationError, errors::messages::FeatureFlagControllerNotSet );
    }

    bool isSuccess = gen.writeOpeningObject();
    {
        isSuccess = isSuccess && gen.writePropertyKey( tokens::Features );
        isSuccess = isSuccess && gen.writeOpeningObject();
        {
            for( const auto& featureFlag : featureFlagController->getFeatureFlagList() ) {
                isSuccess = isSuccess && gen.writePropertyKey( featureFlag.typeStr() );
                isSuccess = isSuccess && gen.writeOpeningObject();
                isSuccess = isSuccess && gen.writeProperty( tokens::Status, featureFlag.statusStr() );
                isSuccess = isSuccess && gen.writeProperty( tokens::Version, featureFlag.version() );
                isSuccess = isSuccess && gen.writeClosingSymbol();
            }
        }
        isSuccess = isSuccess && gen.writeClosingSymbol();
    }
    isSuccess = isSuccess && gen.writeClosingSymbol();

    if( isSuccess ) {
        return std::make_tuple(::errors::NotError, StringType{} );
    }
    else {
        return std::make_tuple(::errors::GenJsonFailed, StringType{} );
    }
}

}  // namespace nma
