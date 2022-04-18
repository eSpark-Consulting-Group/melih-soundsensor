#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "FeatureFlagController.h"

using namespace common::resources;
using namespace core::features;

namespace nma {

WebSocketHandler::ResponceType WebSocketHandler::onHubFeatureStatusSet( const gason::JsonValue& request,
                                                                        common::JsonStreamGenerator& response ) {
    response.writeEmptyObject() && response.writeItemDelimiter();

    auto featureFlagController = FeatureFlagController::get();

    if( !featureFlagController ) {
        return std::make_tuple( errors::ApplicationError, errors::messages::FeatureFlagControllerNotSet );
    }

    auto params = request( tokens::Params );
    auto feature = params( tokens::Feature );
    auto status = params( tokens::Status );

    if( !feature || !status || !feature.isString() || !status.isString() ) {
        return std::make_tuple( errors::InvalidParams, errors::messages::BadParams );
    }

    auto parseFeatureTypeResult = FeatureFlag::typeFromStr( feature.toString() );
    if( !parseFeatureTypeResult.first ) {
        return std::make_tuple( errors::InvalidParams, errors::messages::UnknownFeature );
    }

    auto parseStatusResult = FeatureFlag::statusFromStr( status.toString() );
    if( !parseStatusResult.first ) {
        return std::make_tuple( errors::InvalidParams, errors::messages::UnknownStatus );
    }

    if( !featureFlagController->setFeatureFlag( parseFeatureTypeResult.second, parseStatusResult.second ) ) {
        return std::make_tuple( errors::ApplicationError, errors::messages::CannotSetFeatureFlag );
    }

    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace nma