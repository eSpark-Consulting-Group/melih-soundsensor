
#include "HTTPParser.h"

#include <stdio.h>
#include <cstdlib>
#include <string.h>

#include "DebugUtils.h"

namespace network {

void HTTPParser::checkEndLines( const char currentSymbol ) {
    switch( currentSymbol ) {
    case '\r':
        if( m_hasREnd[0] && m_hasNEnd[0] ) {
            m_hasREnd[1] = true;
        }
        else {
            resetEndMarkers();
            m_hasREnd[0] = true;
        }
        break;

    case '\n':
        if( m_hasNEnd[0] && m_hasREnd[0] && m_hasREnd[1] ) {
            m_hasNEnd[1] = true;
        }
        else if( m_hasREnd[0] ) {
            m_hasNEnd[1] = false;
            m_hasREnd[1] = false;

            m_hasNEnd[0] = true;
        }
        else {
            resetEndMarkers();
        }
        break;

    default:
        resetEndMarkers();
        break;
    };
}

void HTTPParser::resetEndMarkers() {
    m_hasNEnd[0] = false;
    m_hasNEnd[1] = false;

    m_hasREnd[0] = false;
    m_hasREnd[1] = false;
}

void HTTPParser::resetToDefault() {
    resetEndMarkers();

    m_expectedDataSize = 0;
    m_dataSize = 0;
    m_method = HttpMethod::Unknown;
    m_state = State::ParseHeader;
}

void HTTPParser::parseHttpHeader( const char currentSymbol ) {
    checkEndLines( currentSymbol );

    auto buffer = getBuffer();

    buffer[m_dataSize++] = currentSymbol;

    if( hasOneEnd() ) {
        buffer[m_dataSize - 2] = '\0';

        if( strstr( buffer, "POST " ) != nullptr ) {
            m_method = HttpMethod::POST;
            processUrlHeader( buffer, m_dataSize - 1 );
        }
        else if( strstr( buffer, "GET " ) != nullptr ) {
            m_method = HttpMethod::GET;
            processUrlHeader( buffer, m_dataSize - 1 );
        }
        else {
            char key[64] = "";
            char value[161] = "";

            auto result = sscanf( buffer, "%63[^:]: %160[^\r\n]", key, value );
            if( result == 2 ) {
                FUNC_LOGI( "Header:  %s : %s", key, value );
                if( !strcasecmp( key, "Content-Length" ) ) {
                    sscanf( value, "%zu", &m_expectedDataSize );
                    FUNC_LOGI( "Content-Length == %zu", m_expectedDataSize );
                }
            }
        }

        m_dataSize = 0;
    }
    else if( hasDoubleEnd() ) {
        FUNC_LOGI( "Double end msg: %s", buffer );
        if( m_expectedDataSize == 0 ) {
            FUNC_LOGI( "Expected size 0" );
            buffer[0] = '\0';
            processFinishedMessage( buffer, 0 );
            resetToDefault();
            m_state = State::MessageFinished;
        }
        else {
            m_state = State::ParseData;
            m_dataSize = 0;
            resetEndMarkers();
        }
    }
}

void HTTPParser::parseHttpData( const char currentSymbol ) {
    checkEndLines( currentSymbol );

    getBuffer()[m_dataSize++] = currentSymbol;
    if( ( m_dataSize == m_expectedDataSize ) || hasDoubleEnd() ) {
        getBuffer()[m_dataSize] = '\0';
        processFinishedMessage( getBuffer(), m_dataSize );
        resetToDefault();
        m_state = State::MessageFinished;
    }
}

bool HTTPParser::hasDoubleEnd() const {
    return m_hasREnd[0] && m_hasNEnd[0] && m_hasREnd[1] && m_hasNEnd[1];
}

bool HTTPParser::hasOneEnd() const {
    return m_hasREnd[0] && m_hasNEnd[0] && !m_hasREnd[1] && !m_hasNEnd[1];
}

bool HTTPParser::parse( const char* receivedData, std::size_t len ) {
    std::size_t idx = 0;

    if( m_state == State::MessageFinished ) {
        m_state = State::ParseHeader;
    }

    for( ; ( idx < len ) && ( m_dataSize < getMaxBufferSize() ); ++idx ) {
        if( m_state == State::ParseHeader ) {
            parseHttpHeader( receivedData[idx] );
        }
        else if( m_state == State::ParseData ) {
            parseHttpData( receivedData[idx] );
        }
    }

    if( m_dataSize >= getMaxBufferSize() ) {
        FUNC_LOGW( "Internal buffer smaller then received data!" );
        return false;
    }

    return m_state == State::MessageFinished;
}

HTTPParser::HttpMethod HTTPParser::getMethod() {
    return m_method;
}

}  // namespace network
