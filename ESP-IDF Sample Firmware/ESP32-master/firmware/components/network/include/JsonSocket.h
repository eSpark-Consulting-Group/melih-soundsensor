#pragma once

#include "WebSocket.h"
#include "JsonStreamingParser.h"

namespace network {

class JsonSocket : public WebSocket {
public:
    class JsListener : public jssp::JsonListenerBase {
    public:
        JsListener( JsonSocket& handler )
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
        JsonSocket& m_socket;
    };  // class JsListener

    class JsParser : public jssp::JsonStreamingParser {
    public:
        using jssp::JsonStreamingParser::JsonStreamingParser;
    };  // class JsParser

    JsonSocket( ISocket::UPtr&& socket, const bool isSendMask = true );

    int send( const char* srcBuffer, const std::size_t srcBufferSize, const int flags = 0 ) override;

    bool close() override;

    void onJsonDocumentSended();

    void onJsonParseError( int errorCode );

private:
    JsListener m_jsListener;
    JsParser m_jsParser;
    bool m_isParsingFailed = false;
    bool m_isDocumentSended = false;
};

}  // namespace network