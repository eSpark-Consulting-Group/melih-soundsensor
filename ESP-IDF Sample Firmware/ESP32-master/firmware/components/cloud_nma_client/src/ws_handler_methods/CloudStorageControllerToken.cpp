#include "NmaJsonHandler.h"

#if defined( CONFIG_CLOUD_DEVICES ) and !defined( CONFIG_UART_API )

#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "ControllerStorageTokenInfo.h"
#include "CloudNmaClient.h"

#include "DebugUtils.h"

namespace cloud_client {

using namespace common::resources;

constexpr std::chrono::hours DefaultUpdateTimeout{ 24 };

NmaJsonHandler::ResponceType NmaJsonHandler::onCloudStorageControllerToken( const gason::JsonValue& json,
                                                                            common::JsonStreamGenerator& gen ) {
    auto& nmaClient = static_cast<CloudNmaClient&>( m_client );

    auto error = json( tokens::Error )( tokens::Data ).toString();
    if( error ) {
        return std::make_tuple( errors::NotError, StringType() );
    }

    auto result = json( tokens::Result );
    if( !result ) {
        return std::make_tuple( errors::InvalidParams, errors::messages::BadParams );
    }

    core::ControllerStorageToken tokenInfo;

    bool isSuccess;
    if( const gason::JsonValue token = result( tokens::Token ) ) {
        const auto str = token( tokens::String ).toString();
        const auto expires_ts = token( tokens::ExpireTs ).toInt( &isSuccess );
        if( str && isSuccess ) {
            tokenInfo.setTokenString( str );
            tokenInfo.setExpireTs( expires_ts );
            nmaClient.sheduleStorageControllerTokenRequest( std::chrono::seconds{ expires_ts } );
        }
        else {
            nmaClient.sheduleStorageControllerTokenRequest( DefaultUpdateTimeout );
        }
    }
    else {
        nmaClient.sheduleStorageControllerTokenRequest( DefaultUpdateTimeout );
    }

    if( result( tokens::Xapp ) && result( tokens::Xapp ).isString() ) {
        tokenInfo.setXapp( result( tokens::Xapp ).toString() );
    }

    const auto endpointsObj = result( tokens::Endpoints );

    std::list<std::string> uploadEndPoints;
    std::list<std::string> downloadEndPoints;

    if( endpointsObj && endpointsObj.isObject() ) {
        const auto uplObj = endpointsObj( tokens::Upload );
        const auto dwlObj = endpointsObj( tokens::Download );

        if( uplObj && uplObj.isArray() && dwlObj && dwlObj.isArray() ) {
            for( const auto& obj : uplObj ) {
                if( obj.value.isString() ) {
                    const auto& str = obj.value.toString();
                    uploadEndPoints.emplace_back( str );
                }
            }

            for( const auto& obj : dwlObj ) {
                if( obj.value.isString() ) {
                    const auto& str = obj.value.toString();
                    downloadEndPoints.emplace_back( str );
                }
            }
        }

        tokenInfo.setUploadEndpoints( uploadEndPoints );
        tokenInfo.setDownloadEndpoints( downloadEndPoints );

        tokenInfo.save();
    }

    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace cloud_client

#endif  // defined(CONFIG_CLOUD_DEVICES) and !defined(CONFIG_UART_API)
