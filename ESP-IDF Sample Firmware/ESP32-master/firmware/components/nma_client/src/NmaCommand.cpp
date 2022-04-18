#include <NmaCommand.h>
#include <JsonStreamGenerator.h>

#include <CommonResources.h>

#include "Bus.h"
#include "BusCommandNmaClientClbk.h"

#include <string.h>
#include <utility>

namespace nma {

void Command::setSender( SenderInfo&& senderInfo ) {
    m_senderInfo = std::move( senderInfo );
}

bool Command::processActiveToken() {
    return true;
}

bool Command::onStepIn() {
    return true;
}

bool Command::onStepOut() {
    return true;
}

void Command::onSymbolParsed( char /*symbol*/ ) {}

bool Command::onLongToken() {
    return false;
}

void Command::setParser( common::JsonStreamParser* ptrParser ) {
    m_parser = ptrParser;
}

void Command::setDeviceId( std::string&& deviceId ) {
    m_deviceId = std::move( deviceId );
}

bool Command::sendDefaultAnswer() {
    auto clbk = [this]( common::IWriteStream& ws ) -> bool {
        common::JsonStreamGenerator jsonGenerator( ws );

        return writeDefaultAnswerHeader( jsonGenerator ) &&
               jsonGenerator.writePropertyKey( common::resources::tokens::Result ) &&
               jsonGenerator.writeOpeningObject() && jsonGenerator.writeClosingSymbol() && jsonGenerator.writeJsonEnd();
    };
    constexpr auto CmdTimeoutSec = std::chrono::seconds( 5 );
    return bus::waitToResp<bus::command::nma::ClientClbk>( "nma", CmdTimeoutSec, std::move( clbk ) );
}

bool Command::sendErrorAnswer( Command::Errors error, const char* message ) {
    auto clbk = [this, &message, &error]( common::IWriteStream& ws ) -> bool {
        using namespace common::resources;

        common::JsonStreamGenerator jsonGenerator( ws );

        bool result = writeSenderProperties( jsonGenerator );
        result = result && jsonGenerator.writePropertyKey( tokens::Error );
        result = result && jsonGenerator.writeOpeningObject();

        auto errorCode = errors::SystemError;
        switch( error ) {
        case Errors::ApplicationError:
            errorCode = errors::ApplicationError;
            break;

        case Errors::InternalError:
            errorCode = errors::InternalError;
            break;

        case Errors::InvalidParams:
            errorCode = errors::InvalidParams;
            break;

        case Errors::InvalidRequest:
            errorCode = errors::InvalidRequest;
            break;

        case Errors::MethodNotFound:
            errorCode = errors::MethodNotFound;
            break;

        case Errors::ParseError:
            errorCode = errors::ParseError;
            break;

        case Errors::SystemError:
            errorCode = errors::SystemError;
            break;

        case Errors::TransportError:
            errorCode = errors::TransportError;
            break;

        case Errors::GatewayBusy:
            errorCode = errors::ApplicationError;
            if( message == nullptr ) {
                message = errors::messages::GatewayBusy;
            }
            break;
        };

        result = result && jsonGenerator.writeProperty( tokens::Code, errorCode );
        if( message != nullptr ) {
            result = result && jsonGenerator.writeProperty( tokens::Message, message );
        }

        result = result && jsonGenerator.writeClosingSymbol();

        result = result && jsonGenerator.writePropertyKey( tokens::Result );
        result = result && jsonGenerator.writeOpeningObject();
        result = result && jsonGenerator.writeClosingSymbol();

        return result && jsonGenerator.writeJsonEnd();
    };

    constexpr auto CmdTimeoutSec = std::chrono::seconds( 5 );
    return bus::waitToResp<bus::command::nma::ClientClbk>( "nma", CmdTimeoutSec, std::move( clbk ) );
}

bool Command::writeDefaultAnswerHeader( common::JsonStreamGenerator& jsonGenerator ) {
    auto result = jsonGenerator.writeProperty( common::resources::tokens::Error, nullptr );
    result = result && writeSenderProperties( jsonGenerator );
    // result = result && jsonGenerator.writeProperty( common::resources::tokens::Id, getRequestId() );
    return result;
}

bool Command::writeSenderProperties( common::JsonStreamGenerator& jsonGenerator ) {
    if( m_senderInfo.isComplete() ) {
        using namespace common::resources;
        return jsonGenerator.writePropertyKey( tokens::Sender ) && jsonGenerator.writeOpeningObject()

               && jsonGenerator.writeProperty( tokens::ConId, m_senderInfo.getId() ) &&
               jsonGenerator.writeProperty( tokens::Type, m_senderInfo.getType() )

               && jsonGenerator.writeClosingSymbol();
    }

    return true;
}

}  // namespace nma
