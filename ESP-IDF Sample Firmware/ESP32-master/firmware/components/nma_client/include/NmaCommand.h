#pragma once

#include <INmaCommand.h>

#include <NmaSenderInfo.h>
#include <gason.hpp>
#include <cinttypes>
#include <memory>

namespace common {
class JsonStreamParser;
class JsonStreamGenerator;
}  // namespace common

namespace nma {

class Command : public ICommand {
public:
    enum class Errors {
        ApplicationError,
        InternalError,
        InvalidParams,
        InvalidRequest,
        MethodNotFound,
        ParseError,
        SystemError,
        TransportError,
        GatewayBusy,
    };

public:
    Command() = default;
    ~Command() override = default;

public:
    void setSender( SenderInfo&& senderInfo );
    SenderInfo& getSender() {
        return m_senderInfo;
    }

    void setParser( common::JsonStreamParser* ptrParser );

    void setDeviceId( std::string&& requestId );

    bool processActiveToken() override;
    bool onStepIn() override;
    bool onStepOut() override;
    void onSymbolParsed( char symbol ) override;
    bool onLongToken() override;

    bool sendDefaultAnswer();

    bool sendErrorAnswer( Errors error, const char* message = nullptr );

    bool writeSenderProperties( common::JsonStreamGenerator& jsonGenerator );

    bool writeDefaultAnswerHeader( common::JsonStreamGenerator& jsonGenerator );

protected:
    std::string m_deviceId;
    SenderInfo m_senderInfo;

    common::JsonStreamParser* m_parser = nullptr;
};

}  // namespace nma
