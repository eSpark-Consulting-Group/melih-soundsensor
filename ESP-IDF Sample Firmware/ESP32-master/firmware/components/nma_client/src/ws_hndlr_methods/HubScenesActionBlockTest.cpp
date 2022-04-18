#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "DebugUtils.h"
#include "HTTPRequest.h"

namespace nma {

using namespace common::resources;

std::tuple<std::string, std::string, gason::JsonValue, int, std::string> parseNameTypeValue( gason::JsonValue& obj ) {
    std::string name, type;
    const auto strType = obj( tokens::Type ).toString();
    if( !strType ) {
        return std::make_tuple( std::move( name ),
                                std::move( type ),
                                gason::JsonValue(),
                                errors::InvalidParams,
                                "parseNameTypeValue: bad type!" );
    }
    type = strType;

    const auto strName = obj( tokens::Name ).toString();
    if( !strName ) {
        return std::make_tuple( std::move( name ),
                                std::move( type ),
                                gason::JsonValue(),
                                errors::InvalidParams,
                                "parseNameTypeValue: bad name!" );
    }
    name = strName;

    const auto jsValue = obj( tokens::Value );
    if( !jsValue ) {
        return std::make_tuple( std::move( name ),
                                std::move( type ),
                                gason::JsonValue(),
                                errors::InvalidParams,
                                "parseNameTypeValue: bad value!" );
    }

    return std::make_tuple( std::move( name ), std::move( type ), jsValue, errors::NotError, "" );
}

WebSocketHandler::ResponceType WebSocketHandler::onHubScenesActionBlockTest( const gason::JsonValue& json,
                                                                             common::JsonStreamGenerator& jsGen ) {
    auto errorCode = errors::NotError;
    jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();
    auto GetParamsTokenElement = json( tokens::Params );
    std::string name;
    bool result = false;

    if( GetParamsTokenElement ) {
        auto block = json( tokens::Params )( tokens::Block );
        if( block ) {
            auto blockOptions = block( tokens::BlockOptions );
            if( blockOptions ) {
                auto method = blockOptions( tokens::Method );
                if( method ) {
                    name = method( tokens::Name ).toString();
                }
            }
        }

        if( name == modifiers::sendHttpRequest ) {
            const auto fields = block( tokens::Fields );
            std::string url;
            std::string user;
            std::string password;
            std::string request;
            std::string contentType;
            std::string content;
            std::map<std::string, std::string> headers;
            bool skipSecurity = false;
            int error = errors::NotError;

            for( auto jsItr = gason::begin( fields ); jsItr.isValid(); ++jsItr ) {
                std::string name, type;
                gason::JsonValue jsValue;
                std::string errorDescr;
                std::tie( name, type, jsValue, error, errorDescr ) = parseNameTypeValue( jsItr->value );
                if( error != errors::NotError ) {
                    return std::make_tuple( error, errorDescr );
                }

                if( name == tokens::Url && type == tokens::String ) {
                    auto urlCStr = jsValue.toString();
                    if( !urlCStr ) {
                        return std::make_tuple( errors::ParseError, errors::messages::ParseJsonFailed );
                    }
                    url = urlCStr;
                }

                else if( name == tokens::Credential && tokens::Device == tokens::Credential ) {
                    auto userCStr = jsValue( tokens::User ).toString();
                    if( userCStr ) {
                        user = userCStr;
                    }

                    auto passCStr = jsValue( tokens::Password ).toString();
                    if( passCStr ) {
                        password = passCStr;
                    }
                }

                if( name == tokens::Request && type == tokens::String ) {
                    auto reqCStr = jsValue.toString();
                    if( reqCStr ) {
                        request = reqCStr;
                    }
                }

                if( name == tokens::ContentType && type == tokens::String ) {
                    auto contTypeCStr = jsValue.toString();
                    if( contTypeCStr ) {
                        contentType = contTypeCStr;
                    }
                }

                if( name == tokens::Content && type == tokens::String ) {
                    auto contCStr = jsValue.toString();
                    if( !contCStr ) {
                        content = contCStr;
                    }
                }

                if( name == "headers" && type == "dictionary" ) {
                    auto hedersDict = jsValue.toNode();
                    while( hedersDict ) {
                        if( strlen( hedersDict->key ) && hedersDict->value.isString() ) {
                            std::string key( hedersDict->key );
                            std::string value( hedersDict->value.toString() );
                            headers.insert( std::pair<std::string, std::string>( key, value ) );
                            hedersDict = hedersDict->next;
                        }
                    }
                }

                if( name == tokens::SkipSecurity && type == tokens::Bool ) {
                    bool ok;
                    bool val = jsValue.toBool( &ok );
                    if( ok ) {
                        skipSecurity = val;
                    }
                }
            }

            network::HTTPRequest req( url, request );
            req.setCredential( user, password );
            req.setContentType( contentType );
            req.setContent( content );
            req.setHeaders( headers );
            req.setSkipSecurity( skipSecurity );
            int statusCode = req.send();

            result = jsGen.writePropertyKey( tokens::Result );
            result = result && jsGen.writeOpeningObject();
            { result = result && jsGen.writeProperty( "httpAnswerCode", statusCode ); }
            result = result && jsGen.writeClosingSymbol();
        }
    }

    if( !result ) {
        errorCode = errors::ApplicationError;
    }

    return std::make_tuple( errorCode, StringType() );
}

}  // namespace nma
