#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "BusCommandCloudItemValueSet.h"
#include "Utils.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubItemValuePercentApply( const gason::JsonValue& json,
                                                                             common::JsonStreamGenerator& jsGen ) {
    auto jsPercent = json( tokens::Params )( tokens::Percent );
    bool isSuccess = false;
    float percent = jsPercent.toNumber( &isSuccess );
    if( !isSuccess ) {
        jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();
        return std::make_tuple( errors::InvalidParams, errors::data::WrongType );
    }

    isSuccess = ( -100.0 ) <= percent && ( 100.0 ) >= percent;
    if( !isSuccess ) {
        jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();
        return std::make_tuple( errors::InvalidParams, errors::data::RangeInvalid );
    }

    auto ItemId = json( tokens::Params )( tokens::SubId ).toString();
    isSuccess = ItemId && 0 < std::strlen( ItemId );
    if( !isSuccess ) {
        jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();
        return std::make_tuple( errors::InvalidRequest, errors::data::EmptyIdField );
    }

    auto [ivalue, fvalue, errorCode, errorMessage] = common::makeIval( ItemId, percent );
    if( errors::NotError != errorCode ) {
        jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();
        return std::make_tuple( errorCode, std::move( errorMessage ) );
    }

    if( !ivalue ) {
        jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();
        return std::make_tuple( errors::ApplicationError, StringType() );
    }

    decltype( bus::command::CloudItemValueSet::items ) items;
    items.emplace_back( ItemId );
    isSuccess = bus::command::CloudItemValueSet::send( "debug", m_client, std::move( items ), std::move( ivalue ) );
    if( !isSuccess ) {
        jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();
        return std::make_tuple( errors::InternalError, errors::data::EzloInternaleError );
    }

    jsGen.writeOpeningObject() && jsGen.writeProperty( tokens::NewValue, fvalue ) && jsGen.writeClosingSymbol();

    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace nma