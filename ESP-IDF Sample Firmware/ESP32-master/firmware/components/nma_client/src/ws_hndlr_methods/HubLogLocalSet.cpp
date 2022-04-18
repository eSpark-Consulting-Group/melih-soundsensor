#include "NmaWebSocketHandler.h"
#include <gason.hpp>

#include "EspLogger.h"
#include "CommonResources.h"
#include "JsonStreamGenerator.h"

namespace tokens = ::common::resources::tokens;
namespace errors = ::common::resources::errors;

namespace nma {

static std::pair<bool, ::esp_log_level_t> parseLogLevel( const char* level ) {
    if( 0 == ::strcasecmp( level, "ERROR" ) ) {
        return std::make_pair( true, ::esp_log_level_t::ESP_LOG_ERROR );
    }
    else if( 0 == ::strcasecmp( level, "WARNING" ) ) {
        return std::make_pair( true, ::esp_log_level_t::ESP_LOG_WARN );
    }
    else if( 0 == ::strcasecmp( level, "INFO" ) ) {
        return std::make_pair( true, ::esp_log_level_t::ESP_LOG_INFO );
    }
    else if( 0 == ::strcasecmp( level, "DEBUG" ) ) {
        return std::make_pair( true, ::esp_log_level_t::ESP_LOG_DEBUG );
    }
    else if( 0 == ::strcasecmp( level, "TRACE" ) ) {
        return std::make_pair( true, ::esp_log_level_t::ESP_LOG_VERBOSE );
    }
    else {
        return std::make_pair( false, ::esp_log_level_t{} );
    }
}

WebSocketHandler::ResponceType WebSocketHandler::onHubLogLocalSet( const ::gason::JsonValue& request,
                                                                   ::common::JsonStreamGenerator& response ) {
    if( !m_espLogger ) {
        response.writeEmptyObject() && response.writeItemDelimiter();
        return std::make_tuple(::errors::ApplicationError, errors::messages::EspLoggerIsNotSet );
    }

    response.writeEmptyObject() && response.writeItemDelimiter();

    auto globalSeverity = request(::tokens::Params )(::tokens::LogSeverity );

    if( globalSeverity && globalSeverity.isString() ) {
        auto parseResult = parseLogLevel( globalSeverity.toString() );
        if( !parseResult.first ) {
            return std::make_tuple(::errors::InvalidParams, ::errors::data::UnknownLogSeverity );
        }
        m_espLogger->setLogLevel( "*", parseResult.second );
    }

    for( const auto& feature : request( tokens::Params )( tokens::Features ) ) {
        auto logger_name = feature.value( tokens::Name );
        auto severity = feature.value( tokens::LogSeverity );

        if( !logger_name || !logger_name.isString() || !severity || !severity.isString() ) {
            return std::make_tuple(::errors::InvalidParams, ::errors::data::WrongParams );
        }

        auto parseResult = parseLogLevel( severity.toString() );
        if( !parseResult.first ) {
            return std::make_tuple(::errors::InvalidParams, ::errors::data::UnknownLogSeverity );
        }
        m_espLogger->setLogLevel( logger_name.toString(), parseResult.second );
    }

    return std::make_tuple(::errors::NotError, std::string{} );
}

}  // namespace nma
