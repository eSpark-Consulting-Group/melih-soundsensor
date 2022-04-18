#include "UJsonSocket.h"

#include "DebugUtils.h"

namespace network {

UJsonSocket::UJsonSocket( ISocket::UPtr&& socket )
: UWebSocket( std::move( socket ) )
, m_jsListener( *this ) {
    m_jsParser.setListener( &m_jsListener );
}

int UJsonSocket::send( const char* srcBuffer, const std::size_t srcBufferSize, const int flags ) {
    for( std::size_t i = 0; i < srcBufferSize; ++i ) {
        const char* ch = srcBuffer + i;
        m_jsParser.parse( *ch );

        if( m_isParsingFailed ) {
            FUNC_LOGE( "JsonSocket: Parsing incoming json failed" );
            goto exit_by_error;
        }

        if( m_isDocumentSended ) {
            if( 1 != UWebSocket::send( ch, 1, UWebSocket::FinalMessage ) ) {
                FUNC_LOGE( "JsonSocket: Can't send FinalMessage" );
                goto exit_by_error;
            }

            m_isDocumentSended = false;
            m_jsParser.reset();
        }
        else {
            if( 1 != UWebSocket::send( ch, 1, UWebSocket::Default ) ) {
                FUNC_LOGE( "JsonSocket: Can't send data" );
                goto exit_by_error;
            }
        }
    }

    return srcBufferSize;

exit_by_error:
    FUNC_LOGE( "JsonSocket: Connection is forced to close" );
    close();
    return -1;
}

bool UJsonSocket::close() {
    m_jsParser.reset();
    m_isParsingFailed = false;
    m_isDocumentSended = false;

    return UWebSocket::close();
}

void UJsonSocket::onJsonDocumentSended() {
    m_isDocumentSended = true;
}

void UJsonSocket::onJsonParseError( int errorCode ) {
    m_isParsingFailed = true;
}

}  // namespace network