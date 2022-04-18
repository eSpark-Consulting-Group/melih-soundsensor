#include "JsonSocket.h"

#include "DebugUtils.h"

namespace network {

JsonSocket::JsonSocket( ISocket::UPtr&& socket, const bool isSendMask )
: WebSocket( std::move( socket ), isSendMask )
, m_jsListener( *this ) {
    m_jsParser.setListener( &m_jsListener );
}

int JsonSocket::send( const char* srcBuffer, const std::size_t srcBufferSize, const int flags ) {
    for( std::size_t i = 0; i < srcBufferSize; ++i ) {
        const char* ch = srcBuffer + i;
        m_jsParser.parse( *ch );

        if( m_isParsingFailed ) {
            FUNC_LOGE( "JsonSocket: Parsing outcoming json failed" );
            goto exit_by_error;
        }

        if( m_isDocumentSended ) {
            if( 1 != WebSocket::send( ch, 1, WebSocket::FinalMessage ) ) {
                FUNC_LOGE( "JsonSocket: Can't send FinalMessage" );
                goto exit_by_error;
            }

            m_isDocumentSended = false;
            m_jsParser.reset();
        }
        else {
            if( 1 != WebSocket::send( ch, 1, WebSocket::Default ) ) {
                FUNC_LOGE( "JsonSocket: Can't send data" );
                goto exit_by_error;
            }
        }
    }

    return srcBufferSize;

exit_by_error:
    return -1;
}

bool JsonSocket::close() {
    m_jsParser.reset();
    m_isParsingFailed = false;
    m_isDocumentSended = false;

    return WebSocket::close();
}

void JsonSocket::onJsonDocumentSended() {
    m_isDocumentSended = true;
}

void JsonSocket::onJsonParseError( int errorCode ) {
    m_isParsingFailed = true;
}

}  // namespace network
