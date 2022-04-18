/* HTTPClient.cpp */
/* Copyright (C) 2012 mbed.org, MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <HTTPClient.h>

#include <HTTPData/HTTPText.h>
#include <TCPSocket.h>
#include <Utils.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <DebugUtils.h>

#include <stdlib.h>
#include <cstring>
#include <cstdio>
#include <algorithm>

//#include "Utility.h"            // private memory manager
#ifndef UTILITY_H
#define swMalloc malloc  // use the standard
#define swFree free
#endif

#define WARN_AND_ERR_HTTP_CLIENT_LOGS
#if defined( FULL_HTTP_CLIENT_LOGS )

#define HTTP_LOGV( x, ... ) ESP_LOGV( "HTTP", "[VRB %3d] " x, __LINE__, ##__VA_ARGS__ )
#define HTTP_LOGD( x, ... ) ESP_LOGD( "HTTP", "[DBG %3d] " x, __LINE__, ##__VA_ARGS__ )
#define HTTP_LOGI( x, ... ) ESP_LOGI( "HTTP", "[INF %3d] " x, __LINE__, ##__VA_ARGS__ )
#define HTTP_LOGW( x, ... ) ESP_LOGW( "HTTP", "[WRN %3d] " x, __LINE__, ##__VA_ARGS__ )
#define HTTP_LOGE( x, ... ) ESP_LOGE( "HTTP", "[ERR %3d] " x, __LINE__, ##__VA_ARGS__ )

#elif defined( WARN_AND_ERR_HTTP_CLIENT_LOGS )

#define HTTP_LOGV( x, ... ) \
    do {                    \
    } while( false )
#define HTTP_LOGD( x, ... ) \
    do {                    \
    } while( false )
#define HTTP_LOGI( x, ... ) \
    do {                    \
    } while( false )
#define HTTP_LOGW( x, ... ) ESP_LOGW( "HTTP", "[WRN %3d] " x, __LINE__, ##__VA_ARGS__ )
#define HTTP_LOGE( x, ... ) ESP_LOGE( "HTTP", "[ERR %3d] " x, __LINE__, ##__VA_ARGS__ )

#else  // HTTP_CLIENT_LOGS

#define HTTP_LOGV( x, ... ) \
    do {                    \
    } while( false )
#define HTTP_LOGD( x, ... ) \
    do {                    \
    } while( false )
#define HTTP_LOGI( x, ... ) \
    do {                    \
    } while( false )
#define HTTP_LOGW( x, ... ) \
    do {                    \
    } while( false )
#define HTTP_LOGE( x, ... ) \
    do {                    \
    } while( false )

#endif  // HTTP_CLIENT_LOGS

#define wait_ms( TIME ) vTaskDelay( TIME / portTICK_PERIOD_MS );

namespace network {

//#define HTTP_PORT 80
//#define COPY_PSW_AND_USER_NAME

namespace {
constexpr int MaxTriesToReadChunk = 4;
constexpr int MinChunkSize = 512;
constexpr int MaxURLSize = 256; /* Max URL and Max Value for Name:Value */

constexpr char HttpQueryEndLine[] = "\r\n";
constexpr char HttpAuthBearer[] = "Authorization: Bearer ";
constexpr char HttpAuthBasic[] = "Authorization: Basic ";
}  // namespace

HTTPClient::HTTPClient( ISocket& socket )
: m_sock( socket )
, m_basicAuthUser( NULL )
, m_basicAuthPassword( NULL )
, m_nCustomHeaders( 0 )
, m_httpResponseCode( 0 )
, m_maxredirections( 1 )
, m_location( NULL ) {}

HTTPClient::~HTTPClient() {
#ifdef COPY_PSW_AND_USER_NAME
    if( m_basicAuthUser )
        swFree( m_basicAuthUser );
    if( m_basicAuthPassword )
        swFree( m_basicAuthPassword );
#endif
    if( m_location )  // if any redirection was involved, clean up after it.
        swFree( m_location );
    m_location = NULL;  // this step isn't necessary...
}

const char* HTTPClient::GetErrorMessage( HTTPResult res ) {
    const char* msg[static_cast<int8_t>( HTTPResult::Last ) + 1] = {
        "HTTP OK",                    ///< Success
        "HTTP Processing",            ///< Processing
        "HTTP URL Parse error",       ///< url Parse error
        "HTTP DNS error",             ///< Could not resolve name
        "HTTP Protocol error",        ///< Protocol error
        "HTTP 404 Not Found",         ///< HTTP 404 Error
        "HTTP 403 Refused",           ///< HTTP 403 Error
        "HTTP ### Error",             ///< HTTP xxx error
        "HTTP Timeout",               ///< Connection timeout
        "HTTP Connection error",      ///< Connection error
        "HTTP Closed by remote host"  ///< Connection was closed by remote host
    };
    if( static_cast<int8_t>( res ) <= static_cast<int8_t>( HTTPResult::Last ) )
        return msg[static_cast<int8_t>( res )];
    else
        return "HTTP Unknown Code";
};

void HTTPClient::basicAuth( const char* user, const char* password )  // Basic Authentification
{
    m_authMethod = AuthMethod::Basic;

#if COPY_PSW_AND_USER_NAME
    if( m_basicAuthUser )
        swFree( m_basicAuthUser );
    m_basicAuthUser = (char*)swMalloc( strlen( user ) + 1 );
    strcpy( m_basicAuthUser, user );
    if( m_basicAuthPassword )
        swFree( m_basicAuthPassword );
    m_basicAuthPassword = (char*)swMalloc( strlen( password ) + 1 );
    strcpy( m_basicAuthPassword, password );
#else
    m_basicAuthUser = user;
    m_basicAuthPassword = password;
#endif
}

void HTTPClient::bearerAuth( const char* password ) {
    m_authMethod = AuthMethod::Bearer;
#if COPY_PSW_AND_USER_NAME
    if( m_basicAuthPassword )
        swFree( m_basicAuthPassword );
    m_basicAuthPassword = (char*)swMalloc( strlen( password ) + 1 );
    strcpy( m_basicAuthPassword, password );
#else
    m_basicAuthPassword = password;
#endif
}

void HTTPClient::customHeaders( const char** headers, size_t pairs ) {
    m_customHeaders = headers;
    m_nCustomHeaders = pairs;
}

HTTPResult HTTPClient::get( const char* url,
                            IHTTPDataIn* pDataIn,
                            int timeout /*= HTTP_CLIENT_DEFAULT_TIMEOUT*/ )  // Blocking
{
    HTTP_LOGI( "url: %s, timeout: %d", url, timeout );
    return connect( url, HttpMethod::GET, NULL, pDataIn, timeout );
}

HTTPResult HTTPClient::get( const char* url,
                            char* result,
                            size_t maxResultLen,
                            int timeout /*= HTTP_CLIENT_DEFAULT_TIMEOUT*/ )  // Blocking
{
    HTTP_LOGI( "url: %s, maxResultLen %zu, timeout: %d", url, maxResultLen, timeout );
    HTTPText str( result, maxResultLen );
    return get( url, &str, timeout );
}

HTTPResult HTTPClient::post( const char* url,
                             const IHTTPDataOut& dataOut,
                             IHTTPDataIn* pDataIn,
                             int timeout /*= HTTP_CLIENT_DEFAULT_TIMEOUT*/ )  // Blocking
{
    return connect( url, HttpMethod::POST, (IHTTPDataOut*)&dataOut, pDataIn, timeout );
}

HTTPResult HTTPClient::put( const char* url,
                            const IHTTPDataOut& dataOut,
                            IHTTPDataIn* pDataIn,
                            int timeout /*= HTTP_CLIENT_DEFAULT_TIMEOUT*/ )  // Blocking
{
    return connect( url, HttpMethod::PUT, (IHTTPDataOut*)&dataOut, pDataIn, timeout );
}

HTTPResult HTTPClient::del( const char* url,
                            IHTTPDataIn* pDataIn,
                            int timeout /*= HTTP_CLIENT_DEFAULT_TIMEOUT*/ )  // Blocking
{
    return connect( url, HttpMethod::DELETE, NULL, pDataIn, timeout );
}

int HTTPClient::getHTTPResponseCode() {
    return m_httpResponseCode;
}

void HTTPClient::setMaxRedirections( int i ) {
    if( i < 1 )
        i = 1;
    m_maxredirections = i;
}

#define CHECK_CONN_ERR( ret )                               \
    do {                                                    \
        if( ret != HTTPResult::OK ) {                       \
            m_sock.close();                                 \
            HTTP_LOGE( "Connection error (%d)", (int)ret ); \
            return HTTPResult::CONN;                        \
        }                                                   \
    } while( 0 )

#define PRTCL_ERR()                    \
    do {                               \
        m_sock.close();                \
        HTTP_LOGE( "Protocol error" ); \
        return HTTPResult::PRTCL;      \
    } while( 0 )

HTTPResult HTTPClient::connect( const char* url,
                                HttpMethod method,
                                IHTTPDataOut* pDataOut,
                                IHTTPDataIn* pDataIn,
                                int timeout )  // Execute request
{
    m_httpResponseCode = 0;  // Invalidate code
    m_timeout = timeout;

    HTTP_LOGI( "connect(%s,%d,...,%d)", url, (int)method, timeout );
    pDataIn->writeReset();
    if( pDataOut ) {
        pDataOut->readReset();
    }

    char scheme[8];
    uint16_t port;
    char host[128];

    std::array<char, MaxURLSize> pathBuffer;
    std::array<char, MinChunkSize> workflowBuffer;

    auto path = pathBuffer.data();
    auto buf = workflowBuffer.data();

    const auto maxPathSize = pathBuffer.size();
    const auto maxChunkSize = workflowBuffer.size();

    size_t recvContentLength = 0;
    bool recvChunked = false;
    size_t crlfPos = 0;
    size_t trfLen;
    int ret = 0;
    HTTPResult result = HTTPResult::OK;

    int maxRedirect = m_maxredirections;

    while( maxRedirect-- ) {
        bool takeRedirect = false;

        HTTP_LOGI( "parse: [%s]", url );
        // First we need to parse the url (http[s]://host[:port][/[path]]) -- HTTPS not supported (yet?)
        const auto res = parseURL( url, scheme, sizeof( scheme ), host, sizeof( host ), &port, path, maxPathSize );
        if( res != HTTPResult::OK ) {
            HTTP_LOGE( "parseURL returned %d", (int)res );
            return res;
        }

        if( port == 0 ) {
            if( common::stringEndsWith( scheme, "s" ) ) {
                port = 443;
            }
            else {
                port = 80;
            }
        }

        HTTP_LOGD( "Scheme: %s", scheme );
        HTTP_LOGD( "Host: %s", host );
        HTTP_LOGD( "Port: %d", port );
        HTTP_LOGD( "Path: %s", path );

        // Connect
        HTTP_LOGD( "Connecting socket to server" );
        ret = m_sock.connectToHost( host, port );
        if( ret < 0 ) {
            m_sock.close();
            HTTP_LOGE( "Could not connect" );
            return HTTPResult::CONN;
        }

        // Send request
        HTTP_LOGD( "Sending request" );
        const char* methodName = nullptr;

        switch( method ) {
        case HttpMethod::GET:
            methodName = "GET";
            break;

        case HttpMethod::POST:
            methodName = "POST";
            break;

        case HttpMethod::PUT:
            methodName = "PUT";
            break;

        case HttpMethod::DELETE:
            methodName = "DELETE";
            break;

        default:
            methodName = "";
            break;
        };

        snprintf(
            buf, maxChunkSize, "%s %s HTTP/1.1\r\nHost: %s:%d\r\n", methodName, path, host, port );  // Write request
        HTTP_LOGV( " buf{%s}", buf );
        result = send( buf );
        if( result != HTTPResult::OK ) {
            m_sock.close();
            HTTP_LOGE( "Could not write request 1" );
            return HTTPResult::CONN;
        }

        // send authorization
        HTTP_LOGI( "send auth (if defined)" );

        if( m_authMethod == AuthMethod::Basic ) {
            if( m_basicAuthUser && m_basicAuthPassword ) {
                strcpy( buf, "Authorization: Basic " );
                createauth( m_basicAuthUser, m_basicAuthPassword, buf + strlen( buf ), maxChunkSize - strlen( buf ) );
                strcat( buf, HttpQueryEndLine );
                HTTP_LOGI( " (%s,%s) => (%s)", m_basicAuthUser, m_basicAuthPassword, buf );
                result = send( buf );
                HTTP_LOGI( " ret = %d", (int)result );
                if( result != HTTPResult::OK ) {
                    m_sock.close();
                    HTTP_LOGE( "Could not write request 2" );
                    return HTTPResult::CONN;
                }
            }
        }
        else if( m_authMethod == AuthMethod::Bearer ) {
            if( m_basicAuthPassword ) {
                strcpy( buf, HttpAuthBearer );
                strcpy( buf + strlen( buf ), m_basicAuthPassword );
                strcat( buf, HttpQueryEndLine );

                result = send( buf );
                HTTP_LOGI( " ret = %d", (int)result );
                if( result != HTTPResult::OK ) {
                    m_sock.close();
                    HTTP_LOGE( "Could not write request 3" );
                    return HTTPResult::CONN;
                }
            }
        }

        // Send all headers
        HTTP_LOGI( "Send custom header(s) %zu (if any)", m_nCustomHeaders );
        for( size_t nh = 0; nh < m_nCustomHeaders * 2; nh += 2 ) {
            HTTP_LOGI( "hdr[%2zu] %s: %s", nh, m_customHeaders[nh], m_customHeaders[nh + 1] );
            snprintf( buf, maxChunkSize, "%s: %s\r\n", m_customHeaders[nh], m_customHeaders[nh + 1] );
            result = send( buf );
            if( result != HTTPResult::OK ) {
                HTTP_LOGE( "closing" );
                wait_ms( 50 );
                m_sock.close();
                HTTP_LOGE( "Could not write request 4" );
                return HTTPResult::CONN;
            }
            HTTP_LOGI( "   send() returned %d", (int)result );
        }

        // Send default headers
        HTTP_LOGD( "Sending headers" );
        if( pDataOut != NULL ) {
            char type[48];
            if( pDataOut->getDataType( type, 48 ) == HTTPDataResult::OK ) {
                snprintf( buf, maxChunkSize, "Content-Type: %s\r\n", type );
                result = send( buf );
                CHECK_CONN_ERR( result );
            }

            if( pDataOut->getIsChunked() ) {
                result = send( "Transfer-Encoding: chunked\r\n" );
                CHECK_CONN_ERR( result );
            }
            else {
                snprintf( buf, maxChunkSize, "Content-Length: %zu\r\n", pDataOut->getDataLen() );
                result = send( buf );
                CHECK_CONN_ERR( result );
            }
        }

        // Close headers
        HTTP_LOGD( "Headers sent" );
        result = send( "\r\n" );
        CHECK_CONN_ERR( result );

        // Send data (if available)
        if( pDataOut != NULL ) {
            HTTP_LOGV( "Sending data [%s]", buf );
            while( true ) {
                size_t writtenLen = 0;
                pDataOut->read( buf, maxChunkSize, &trfLen );
                HTTP_LOGV( "  trfLen: %zu", trfLen );
                if( pDataOut->getIsChunked() ) {
                    // Write chunk header
                    char chunkHeader[16];
                    snprintf( chunkHeader, sizeof( chunkHeader ), "%zX\r\n", trfLen );  // In hex encoding
                    result = send( chunkHeader );
                    CHECK_CONN_ERR( result );
                }
                else if( trfLen == 0 ) {
                    break;
                }
                if( trfLen != 0 ) {
                    result = send( buf, trfLen );
                    CHECK_CONN_ERR( result );
                }

                if( pDataOut->getIsChunked() ) {
                    result = send( "\r\n" );  // Chunk-terminating CRLF
                    CHECK_CONN_ERR( result );
                }
                else {
                    writtenLen += trfLen;
                    if( writtenLen >= pDataOut->getDataLen() ) {
                        break;
                    }
                }

                if( trfLen == 0 ) {
                    break;
                }
            }

            pDataOut->readReset();
        }

        // Receive response
        HTTP_LOGD( "Receiving response" );
        // ret = recv(buf, maxChunkSize - 1, maxChunkSize - 1, &trfLen); //Read n bytes
        result = recv( buf, 1, maxChunkSize - 1, &trfLen );  // recommended by Rob Noble to avoid timeout wait
        CHECK_CONN_ERR( result );
        buf[trfLen] = '\0';
        HTTP_LOGV( "Received \r\n(%s\r\n)", buf );

        char* crlfPtr = strstr( buf, "\r\n" );
        if( crlfPtr == NULL ) {
            PRTCL_ERR();
        }

        crlfPos = crlfPtr - buf;
        buf[crlfPos] = '\0';

        // Parse HTTP response
        if( sscanf( buf, "HTTP/%*d.%*d %d %*[^\r\n]", &m_httpResponseCode ) != 1 ) {
            // Cannot match string, error
            HTTP_LOGE( "Not a correct HTTP answer : {%s}\n", buf );
            PRTCL_ERR();
        }

        if( ( m_httpResponseCode < 200 ) || ( m_httpResponseCode >= 400 ) ) {
            // Did not return a 2xx code; TODO fetch headers/(&data?) anyway and implement a mean of writing/reading
            // headers
            HTTP_LOGW( "Response code %d", m_httpResponseCode );
            PRTCL_ERR();
        }

        HTTP_LOGD( "Reading headers" );

        memmove(
            buf, &buf[crlfPos + 2], trfLen - ( crlfPos + 2 ) + 1 );  // Be sure to move NULL-terminating char as well
        trfLen -= ( crlfPos + 2 );

        recvContentLength = 0;
        recvChunked = false;
        // Now get headers
        while( true ) {
            crlfPtr = strstr( buf, "\r\n" );
            if( crlfPtr == NULL ) {
                if( trfLen < maxChunkSize - 1 ) {
                    size_t newTrfLen = 0;
                    result = recv( buf + trfLen, 1, maxChunkSize - trfLen - 1, &newTrfLen );
                    trfLen += newTrfLen;
                    buf[trfLen] = '\0';
                    HTTP_LOGV( "Read %zu chars; In buf: [%s]", newTrfLen, buf );
                    CHECK_CONN_ERR( result );
                    continue;
                }
                else {
                    PRTCL_ERR();
                }
            }

            crlfPos = crlfPtr - buf;

            if( crlfPos == 0 ) {  // End of headers
                HTTP_LOGD( "Headers read" );
                memmove( buf, &buf[2], trfLen - 2 + 1 );  // Be sure to move NULL-terminating char as well
                trfLen -= 2;
                break;
            }

            buf[crlfPos] = '\0';

            char key[64];
            std::array<char, MaxURLSize> valueBuffer;

            auto value = valueBuffer.data();
            const auto valueMaxSize = valueBuffer.size();

            key[sizeof( key ) - 1] = '\0';
            value[valueMaxSize - 1] = '\0';

            int n = sscanf( buf, "%63[^:]: %160[^\r\n]", key, value );
            if( n == 2 ) {
                HTTP_LOGD( "Read header : %s: %s", key, value );
                if( !strcasecmp( key, "Content-Length" ) ) {
                    sscanf( value, "%zu", &recvContentLength );
                    pDataIn->setDataLen( recvContentLength );
                }
                else if( !strcasecmp( key, "Transfer-Encoding" ) ) {
                    if( !strcasecmp( value, "Chunked" ) ) {
                        recvChunked = true;
                        pDataIn->setIsChunked( true );
                    }
                }
                else if( !strcasecmp( key, "Content-Type" ) ) {
                    pDataIn->setDataType( value );
                }
                else if( !strcasecmp( key, "Location" ) ) {
                    if( m_location ) {
                        swFree( m_location );
                    }
                    m_location = (char*)swMalloc( strlen( value ) + 1 );
                    if( m_location ) {
                        strcpy( m_location, value );
                        url = m_location;
                        HTTP_LOGI( "Following redirect[%d] to [%s]", maxRedirect, url );
                        m_sock.close();
                        takeRedirect = true;
                        break;  // exit the while(true) header to follow the redirect
                    }
                    else {
                        HTTP_LOGE( "Could not allocate memory for %s", key );
                    }
                }

                memmove( buf,
                         &buf[crlfPos + 2],
                         trfLen - ( crlfPos + 2 ) + 1 );  // Be sure to move NULL-terminating char as well
                trfLen -= ( crlfPos + 2 );
            }
            else {
                HTTP_LOGE( "Could not parse header" );
                PRTCL_ERR();
            }

        }  // while(true) // get headers
        if( !takeRedirect )
            break;
    }  // while (maxRedirect)

    HTTPResult res = HTTPResult::OK;
    if( recvContentLength > 0 ) {
        res =
            receiveDataWithContentLength( buf, trfLen, recvContentLength, recvChunked, workflowBuffer.size(), pDataIn );
    }
    else {
        // if header does not contain Content-Length
        res = receiveDataWithoutContentLength(
            buf, trfLen, recvContentLength, recvChunked, workflowBuffer.size(), pDataIn );
    }

    m_sock.close();
    HTTP_LOGD( "Completed HTTP transaction" );
    return res;
}

HTTPResult HTTPClient::receiveDataWithContentLength( char* buf,
                                                     size_t trfLen,
                                                     size_t recvContentLength,
                                                     bool recvChunked,
                                                     size_t maxChunkSize,
                                                     IHTTPDataIn* pDataIn ) {
    HTTPResult result = HTTPResult::OK;

    if( buf == nullptr || pDataIn == nullptr ) {
        return HTTPResult::ERROR;
    }
    size_t crlfPos = 0;

    // Receive data
    HTTP_LOGV( "Receiving data" );
    while( true ) {
        size_t readLen = 0;

        if( recvChunked ) {
            // Read chunk header
            bool foundCrlf;
            do {
                foundCrlf = false;
                crlfPos = 0;
                buf[trfLen] = 0;
                if( trfLen >= 2 ) {
                    for( ; crlfPos < trfLen - 2; crlfPos++ ) {
                        if( buf[crlfPos] == '\r' && buf[crlfPos + 1] == '\n' ) {
                            foundCrlf = true;
                            break;
                        }
                    }
                }
                if( !foundCrlf ) {  // Try to read more
                    if( trfLen < maxChunkSize ) {
                        size_t newTrfLen = 0;
                        result = recv( buf + trfLen, 0, maxChunkSize - trfLen - 1, &newTrfLen );
                        trfLen += newTrfLen;
                        CHECK_CONN_ERR( result );
                        continue;
                    }
                    else {
                        PRTCL_ERR();
                    }
                }
            } while( !foundCrlf );
            buf[crlfPos] = '\0';
            int n = sscanf( buf, "%zx", &readLen );
            if( n != 1 ) {
                HTTP_LOGE( "Could not read chunk length" );
                PRTCL_ERR();
            }

            memmove(
                buf, &buf[crlfPos + 2], trfLen - ( crlfPos + 2 ) );  // Not need to move NULL-terminating char any more
            trfLen -= ( crlfPos + 2 );

            if( readLen == 0 ) {
                // Last chunk
                break;
            }
        }
        else {
            readLen = recvContentLength;
        }

        HTTP_LOGV( "Retrieving %zu bytes", readLen );

        do {
            HTTP_LOGV( "write %zu,%zu: %s", trfLen, readLen, buf );
            pDataIn->write( buf, std::min( trfLen, readLen ) );
            if( trfLen > readLen ) {
                memmove( buf, &buf[readLen], trfLen - readLen );
                trfLen -= readLen;
                readLen = 0;
            }
            else {
                readLen -= trfLen;
            }

            if( readLen ) {
                result = recv( buf, 1, maxChunkSize - trfLen - 1, &trfLen );
                CHECK_CONN_ERR( result );
                HTTP_LOGV( "recv'd next chunk ret: %d", static_cast<int8_t>( result ) );
            }
        } while( readLen );

        if( recvChunked ) {
            if( trfLen < 2 ) {
                size_t newTrfLen;
                // Read missing chars to find end of chunk
                HTTP_LOGV( "read chunk" );
                result = recv( buf + trfLen, 2 - trfLen, maxChunkSize - trfLen - 1, &newTrfLen );
                CHECK_CONN_ERR( result );
                trfLen += newTrfLen;
                HTTP_LOGV( "recv'd next chunk ret: %d", static_cast<int8_t>( result ) );
            }
            if( ( buf[0] != '\r' ) || ( buf[1] != '\n' ) ) {
                HTTP_LOGE( "Format error" );
                PRTCL_ERR();
            }
            memmove( buf, &buf[2], trfLen - 2 );
            trfLen -= 2;
        }
        else {
            break;
        }
    }
    return result;
}

HTTPResult HTTPClient::receiveDataWithoutContentLength( char* buf,
                                                        size_t trfLen,
                                                        size_t recvContentLength,
                                                        bool recvChunked,
                                                        size_t maxChunkSize,
                                                        IHTTPDataIn* pDataIn ) {
    HTTPResult result = HTTPResult::OK;

    if( buf == nullptr || pDataIn == nullptr ) {
        return HTTPResult::ERROR;
    }

    size_t crlfPos = 0;

    // Receive data
    HTTP_LOGD( "Receiving data" );
    while( true ) {
        size_t readLen = 0;

        if( recvChunked ) {
            // Read chunk header
            bool foundCrlf;
            uint8_t counter = 0;
            do {
                foundCrlf = false;
                crlfPos = 0;
                buf[trfLen] = 0;
                if( trfLen >= 2 ) {
                    for( ; crlfPos < trfLen - 2; crlfPos++ ) {
                        if( buf[crlfPos] == '\r' && buf[crlfPos + 1] == '\n' ) {
                            foundCrlf = true;
                            break;
                        }
                    }
                }
                if( !foundCrlf ) {  // Try to read more
                    if( trfLen < maxChunkSize ) {
                        size_t newTrfLen = 0;
                        result = recv( buf + trfLen, 0, maxChunkSize - trfLen - 1, &newTrfLen );
                        trfLen += newTrfLen;
                        CHECK_CONN_ERR( result );
                        counter++;
                        if( counter < MaxTriesToReadChunk ) {
                            continue;
                        }
                        else {
                            break;
                        }
                    }
                    else {
                        PRTCL_ERR();
                    }
                }
            } while( !foundCrlf );

            if( counter < MaxTriesToReadChunk ) {
                buf[crlfPos] = '\0';
                int n = sscanf( buf, "%zx", &readLen );
                if( n != 1 ) {
                    HTTP_LOGE( "Could not read chunk length" );
                    PRTCL_ERR();
                }
                memmove( buf,
                         &buf[crlfPos + 2],
                         trfLen - ( crlfPos + 2 ) );  // Not need to move NULL-terminating char any more
                trfLen -= ( crlfPos + 2 );

                if( readLen == 0 && counter < MaxTriesToReadChunk ) {
                    // Last chunk
                    HTTP_LOGD( "Last chunk has been detected. Break" );
                    break;
                }
            }
            else {
                HTTP_LOGV( "crlfPos %zu bytes", crlfPos );
                HTTP_LOGV( "trfLen %zu bytes", trfLen );
                readLen = crlfPos + 2;
                buf[readLen] = '\0';
            }
        }
        else {
            readLen = recvContentLength;
        }

        HTTP_LOGV( "Retrieving %zu bytes", readLen );

        do {
            HTTP_LOGV( "write %zu,%zu: %s", trfLen, readLen, buf );
            HTTPDataResult parsRes = pDataIn->write( buf, std::min( trfLen, readLen ) );
            if( parsRes == HTTPDataResult::OK ) {
                readLen = 0;
                recvChunked = false;
                break;
            }

            if( trfLen > readLen ) {
                memmove( buf, &buf[readLen], trfLen - readLen );
                trfLen -= readLen;
                readLen = 0;
            }
            else {
                readLen -= trfLen;
            }

            if( readLen ) {
                result = recv( buf, 1, maxChunkSize - trfLen - 1, &trfLen );
                CHECK_CONN_ERR( result );
                HTTP_LOGV( "recv'd next chunk ret: %d", static_cast<int8_t>( result ) );
            }
        } while( readLen );

        if( recvChunked ) {
            if( recvContentLength == 0 ) {
                trfLen = 0;
                continue;
            }

            if( trfLen < 2 ) {
                size_t newTrfLen;
                // Read missing chars to find end of chunk
                HTTP_LOGV( "read chunk" );
                result = recv( buf + trfLen, 2 - trfLen, maxChunkSize - trfLen - 1, &newTrfLen );
                CHECK_CONN_ERR( result );
                trfLen += newTrfLen;
                HTTP_LOGV( "recv'd next chunk ret: %d", static_cast<int8_t>( result ) );
            }
            if( ( buf[0] != '\r' ) || ( buf[1] != '\n' ) ) {
                HTTP_LOGE( "Format error" );
                PRTCL_ERR();
            }
            memmove( buf, &buf[2], trfLen - 2 );
            trfLen -= 2;
        }
        else {
            break;
        }
    }

    return result;
}

HTTPResult HTTPClient::recv( char* buf,
                             size_t minLen,
                             size_t maxLen,
                             size_t* pReadLen )  // 0 on success, err code on failure
{
    HTTP_LOGV( "Trying to read between %zu and %zu bytes", minLen, maxLen );
    size_t readLen = 0;

    if( !m_sock.isConnected() ) {
        HTTP_LOGW( "Connection was closed by server 1" );
        return HTTPResult::CLOSED;
    }

    int ret;
    while( readLen < maxLen ) {
        if( readLen < minLen ) {
            HTTP_LOGV( "Trying to read at most %4zu bytes [not Blocking, %d] %zu,%zu",
                       minLen - readLen,
                       m_timeout,
                       minLen,
                       readLen );

            // m_sock->setBlocking( false, m_timeout );
            // ret = m_sock->receiveAll( buf + readLen, minLen - readLen );
            m_sock.setReadTimeout( std::chrono::milliseconds( m_timeout ) );
            ret = m_sock.receive( buf + readLen, minLen - readLen );
            if( 0 == ret ) {
                FUNC_LOGE( "New data were not received within timeout. Close connection" );
                m_sock.close();
                ret = -1;
            }
        }
        else {
            HTTP_LOGV(
                "Trying to read at most %4zu bytes [Not blocking, %d] %zu,%zu", maxLen - readLen, 0, maxLen, readLen );
            // m_sock->setBlocking( false, 0 );
            m_sock.setReadTimeout( std::chrono::milliseconds( m_timeout / 10 ) );
            ret = m_sock.receive( buf + readLen, maxLen - readLen );
        }

        if( ret > 0 ) {
            readLen += ret;
        }
        else if( ret == 0 ) {
            break;
        }
        else {
            if( !m_sock.isConnected() ) {
                HTTP_LOGE( "Connection error (recv returned %d)", ret );
                *pReadLen = readLen;
                return HTTPResult::CONN;
            }
            else {
                break;
            }
        }
        if( !m_sock.isConnected() ) {
            break;
        }
    }
    HTTP_LOGV( "Read %zu bytes", readLen );
    buf[readLen] = '\0';  // DS makes it easier to see what's new.
    *pReadLen = readLen;
    return HTTPResult::OK;
}

HTTPResult HTTPClient::send( const char* buf, size_t len ) {  // 0 on success, err code on failure
    if( len == 0 ) {
        len = strlen( buf );
    }
    HTTP_LOGV( "send(\r\n%s,%zu)", buf, len );
    size_t writtenLen = 0;

    if( m_sock.sendAll( buf, len ) ) {
        writtenLen += len;
    }
    else {
        HTTP_LOGW( "Connection was closed because server not responded" );
        return HTTPResult::CLOSED;
    }

    HTTP_LOGV( "Written %zu bytes", writtenLen );
    return HTTPResult::OK;
}

HTTPResult HTTPClient::parseURL( const char* url,
                                 char* scheme,
                                 size_t maxSchemeLen,
                                 char* host,
                                 size_t maxHostLen,
                                 uint16_t* port,
                                 char* path,
                                 size_t maxPathLen ) {  // Parse URL
    char* schemePtr = (char*)url;
    char* hostPtr = (char*)strstr( url, "://" );
    HTTP_LOGI(
        "parseURL(%s,%p,%zu,%s,%zu,%d,%p,%zu", url, scheme, maxSchemeLen, host, maxHostLen, *port, path, maxPathLen );
    if( hostPtr == NULL ) {
        HTTP_LOGW( "Could not find host" );
        return HTTPResult::PARSE;  // URL is invalid
    }

    if( (uint16_t)maxSchemeLen < hostPtr - schemePtr + 1 ) {  // including NULL-terminating char
        HTTP_LOGW( "Scheme str is too small (%zu >= %zu)", maxSchemeLen, size_t( hostPtr - schemePtr + 1 ) );
        return HTTPResult::PARSE;
    }
    memcpy( scheme, schemePtr, hostPtr - schemePtr );
    scheme[hostPtr - schemePtr] = '\0';

    hostPtr += 3;

    size_t hostLen = 0;

    char* portPtr = strchr( hostPtr, ':' );
    if( portPtr != NULL ) {
        hostLen = portPtr - hostPtr;
        portPtr++;
        if( sscanf( portPtr, "%hu", port ) != 1 ) {
            HTTP_LOGW( "Could not find port" );
            return HTTPResult::PARSE;
        }
    }
    else {
        *port = 0;
    }
    HTTP_LOGI( "  hostPtr: %s", hostPtr );
    HTTP_LOGI( "  hostLen: %zu", hostLen );
    char* pathPtr = strchr( hostPtr, '/' );
    if( pathPtr == 0 ) {
        HTTP_LOGW( "The authority component of a URI is not terminated with a slash: %s", hostPtr );
        return HTTPResult::PARSE;
    }
    if( hostLen == 0 ) {
        hostLen = pathPtr - hostPtr;
    }

    if( maxHostLen < hostLen + 1 ) {  // including NULL-terminating char
        HTTP_LOGW( "Host str is too small (%zu >= %zu)", maxHostLen, size_t( hostLen + 1 ) );
        return HTTPResult::PARSE;
    }
    memcpy( host, hostPtr, hostLen );
    host[hostLen] = '\0';

    size_t pathLen;
    char* fragmentPtr = strchr( hostPtr, '#' );
    if( fragmentPtr != NULL ) {
        pathLen = fragmentPtr - pathPtr;
    }
    else {
        pathLen = strlen( pathPtr );
    }

    if( maxPathLen < pathLen + 1 ) {  // including NULL-terminating char
        HTTP_LOGW( "Path str is too small (%zu >= %zu)", maxPathLen, size_t( pathLen + 1 ) );
        return HTTPResult::PARSE;
    }
    memcpy( path, pathPtr, pathLen );
    path[pathLen] = '\0';

    return HTTPResult::OK;
}

void HTTPClient::createauth( const char* user, const char* pwd, char* buf, int len ) {
    char tmp[80];

    snprintf( tmp, sizeof( tmp ), "%s:%s", user, pwd );
    base64enc( tmp, strlen( tmp ), &buf[strlen( buf )], len - strlen( buf ) );
}

// Copyright (c) 2010 Donatien Garnier (donatiengar [at] gmail [dot] com)
int HTTPClient::base64enc( const char* input, unsigned int length, char* output, int len ) {
    static const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned int c, c1, c2, c3;

    if( (uint16_t)len < ( ( ( ( length - 1 ) / 3 ) + 1 ) << 2 ) )
        return -1;
    for( unsigned int i = 0, j = 0; i < length; i += 3, j += 4 ) {
        c1 = ( ( ( (unsigned char)*( (unsigned char*)&input[i] ) ) ) );
        c2 = ( length > i + 1 ) ? ( ( ( (unsigned char)*( (unsigned char*)&input[i + 1] ) ) ) ) : 0;
        c3 = ( length > i + 2 ) ? ( ( ( (unsigned char)*( (unsigned char*)&input[i + 2] ) ) ) ) : 0;

        c = ( ( c1 & 0xFC ) >> 2 );
        output[j + 0] = base64[c];
        c = ( ( c1 & 0x03 ) << 4 ) | ( ( c2 & 0xF0 ) >> 4 );
        output[j + 1] = base64[c];
        c = ( ( c2 & 0x0F ) << 2 ) | ( ( c3 & 0xC0 ) >> 6 );
        output[j + 2] = ( length > i + 1 ) ? base64[c] : '=';
        c = ( c3 & 0x3F );
        output[j + 3] = ( length > i + 2 ) ? base64[c] : '=';
    }
    output[( ( ( length - 1 ) / 3 ) + 1 ) << 2] = '\0';
    return 0;
}

}  // namespace network
