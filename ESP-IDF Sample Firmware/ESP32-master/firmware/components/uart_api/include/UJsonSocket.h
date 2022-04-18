#pragma once

#include "UWebSocket.h"
#include "JsonStreamingParser.h"

namespace network {

class UJsonSocket : public UWebSocket {
public:
    class UJsListener : public jssp::JsonListenerBase {
    public:
        UJsListener( UJsonSocket& handler )
        : m_socket( handler ) {}

        void startDocument() override{
            // Nothing to do
        };

        void endDocument() override {
            m_socket.onJsonDocumentSended();
        };

        void error( int errorCode ) override {
            m_socket.onJsonParseError( errorCode );
        }

    private:
        UJsonSocket& m_socket;
    };

    class JsParser : public jssp::JsonStreamingParser {
    public:
        using jssp::JsonStreamingParser::JsonStreamingParser;
    };

    UJsonSocket( ISocket::UPtr&& socket );

    int send( const char* srcBuffer, const std::size_t srcBufferSize, const int flags = 0 ) override;

    bool close() override;

    void onJsonDocumentSended();

    void onJsonParseError( int errorCode );

private:
    UJsListener m_jsListener;
    JsParser m_jsParser;
    bool m_isParsingFailed = false;
    bool m_isDocumentSended = false;
};

}  // namespace network