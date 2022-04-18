#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "Ezlo.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubReset( const gason::JsonValue& json,
                                                             common::JsonStreamGenerator& jsGen ) {
    jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();

    auto ezlo = core::Ezlo::get();
    if( !ezlo ) {
        return std::make_tuple( errors::InternalError, StringType() );
    }

    auto errorCode = errors::NotError;
    if( json( tokens::Params )( tokens::ResetToFactoryDefaults ).toBool() ) {
        ezlo->startReset( core::Ezlo::ResetType::ResetToFactoryDefaults );
    }
    else if( json( tokens::Params )( tokens::SoftReset ).toBool() ) {
        ezlo->startReset( core::Ezlo::ResetType::SoftReset );
    }
    else {
        errorCode = errors::InvalidParams;
    }

    return std::make_tuple( errorCode, StringType() );
}

}  // namespace nma