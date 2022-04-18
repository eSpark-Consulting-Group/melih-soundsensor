#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "RemoteLogger.h"

#if CONFIG_REMOTE_LOGGING
#ifdef IS_SAVE_CRASH_DUMP
#include "BusCommandSendCrashLog.h"
#endif  // IS_SAVE_CRASH_DUMP
#endif  // CONFIG_REMOTE_LOGGING

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubLogRemoteSet( const gason::JsonValue& json,
                                                                    common::JsonStreamGenerator& jsGen ) {
    auto errorCode = errors::NotError;
    StringType errorMessage;
    jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();

    bool is_enable = json( tokens::Params )( tokens::LogEnable ).toBool();
#if CONFIG_REMOTE_LOGGING
    auto logger = core::Logger::get();
    if( !logger ) {
        return std::make_tuple( errors::InternalError, StringType() );
    }
    if( is_enable ) {
        logger->setLogLevel( json( tokens::Params )( tokens::LogSeverity ).toString() );
#ifdef IS_SAVE_CRASH_DUMP
        auto send_crash_log = std::make_unique<bus::command::SendCrashLog>();
        if( send_crash_log ) {
            m_client.send( m_client.addr(), std::move( send_crash_log ) );
        }
#endif  // IS_SAVE_CRASH_DUMP
    }
    else {
        logger->setLogLevel( core::Logger::NONE );
    }
#else
    errorCode = errors::InvalidRequest;
    errorMessage = errors::messages::RemoteLoggingIsDisabled;
#endif  // CONFIG_REMOTE_LOGGING
    return std::make_tuple( errorCode, std::move( errorMessage ) );
}

}  // namespace nma