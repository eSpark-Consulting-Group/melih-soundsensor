#include "WebSocket.h"

#include "Ezlo.h"
#include "Utils.h"
#include "Thread.h"

#include <sstream>
#include <iomanip>

#include "DebugUtils.h"

#include <sys/socket.h>
#ifdef CONFIG_SPIRAM
#include "esp_heap_caps.h"
#endif  // CONFIG_SPIRAM

#include <string.h>
#include <cstdio>
#include <limits>

namespace network {

namespace {

static const int WEBSOCKET_MIN_LIMIT = 126;
static const int WEBSOCKET_NEXT_LIMIT = 127;

// WebSocket op codes as found in a WebSocket frame.
static const int OPCODE_CONTINUE = 0x00;
static const int OPCODE_TEXT = 0x01;
static const int OPCODE_BINARY = 0x02;
static const int OPCODE_CLOSE = 0x08;
static const int OPCODE_PING = 0x09;
static const int OPCODE_PONG = 0x0a;

static constexpr uint16_t CLOSE_NORMAL_CLOSURE = 1000;
static constexpr uint16_t CLOSE_GOING_AWAY = 1001;
static constexpr uint16_t CLOSE_PROTOCOL_ERROR = 1002;
static constexpr uint16_t CLOSE_CANNOT_ACCEPT = 1003;
static constexpr uint16_t CLOSE_NO_STATUS_CODE = 1005;
static constexpr uint16_t CLOSE_CLOSED_ABNORMALLY = 1006;
static constexpr uint16_t CLOSE_NOT_CONSISTENT = 1007;
static constexpr uint16_t CLOSE_VIOLATED_POLICY = 1008;
static constexpr uint16_t CLOSE_TOO_BIG = 1009;
static constexpr uint16_t CLOSE_NO_EXTENSION = 1010;
static constexpr uint16_t CLOSE_UNEXPECTED_CONDITION = 1011;
static constexpr uint16_t CLOSE_SERVICE_RESTART = 1012;
static constexpr uint16_t CLOSE_TRY_AGAIN_LATER = 1013;
static constexpr uint16_t CLOSE_TLS_HANDSHAKE_FAILURE = 1015;

static constexpr std::chrono::seconds ReceiveMaxPeriod( 60 );

static constexpr std::chrono::seconds HandshakeReceiveTimeout( 3 );
static constexpr std::chrono::seconds HandshakeSendTimeout( 5 );

static constexpr std::chrono::milliseconds DefaultReciveTimeout( 10 );
static constexpr std::chrono::seconds DefaultSendTimeout( 16 );

#define WEB_SOCK_LOG

#ifdef WEB_SOCK_LOG

#define WS_TAG "[WEB_SOCK"
#define WS_LOGE( _FMT, ... ) ESP_LOGE( WS_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )
#define WS_LOGW( _FMT, ... ) ESP_LOGW( WS_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )
#define WS_LOGI( _FMT, ... ) ESP_LOGI( WS_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )
#define WS_LOGD( _FMT, ... ) ESP_LOGD( WS_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )

#else

#define WS_LOGE( ... ) \
    do {               \
    } while( false )
#define WS_LOGW( ... ) \
    do {               \
    } while( false )
#define WS_LOGI( ... ) \
    do {               \
    } while( false )
#define WS_LOGD( ... ) \
    do {               \
    } while( false )

#endif

// Structure definition for the WebSocket frame.
struct Frame {
    // Byte 0
    uint8_t opCode : 4;  // [7:4]
    uint8_t rsv3 : 1;    // [3]
    uint8_t rsv2 : 1;    // [2]
    uint8_t rsv1 : 1;    // [1]
    uint8_t fin : 1;     // [0]

    // Byte 1
    uint8_t len : 7;   // [7:1]
    uint8_t mask : 1;  // [0]

    bool send( ISocket& sock ) const {
        return sock.sendAll( reinterpret_cast<const char*>( this ), sizeof( *this ) );
    }
};

class WsFrame {
    Frame m_frame;
    union {
        std::uint16_t m_length16;
        std::uint64_t m_length64;
    };
    char* m_buffer;
    union {
        mutable std::uint32_t m_sendMask32b = 0x12A7B111;
        std::uint8_t m_sendMaskBytes[sizeof( m_sendMask32b )];
    };

public:
    WsFrame( char buffer[], const unsigned int length, bool isApplyMask )
    : m_buffer( buffer ) {
        m_frame.rsv1 = 0;
        m_frame.rsv2 = 0;
        m_frame.rsv3 = 0;
        m_frame.mask = isApplyMask ? 1 : 0;
        m_frame.opCode = OPCODE_PONG;
        m_frame.fin = 1;
        m_frame.len = length;
    }

    WsFrame( const bool isFirstFrame,
             const bool isFinalFrame,
             char buffer[],
             const unsigned int length,
             bool isApplyMask )
    : m_buffer( buffer ) {
        m_frame.rsv1 = 0;
        m_frame.rsv2 = 0;
        m_frame.rsv3 = 0;
        m_frame.mask = isApplyMask ? 1 : 0;
        m_frame.opCode = isFirstFrame ? OPCODE_TEXT : OPCODE_CONTINUE;
        m_frame.fin = isFinalFrame ? 1 : 0;

        if( 126 > length ) {
            m_frame.len = length;
        }
        else if( std::numeric_limits<std::uint16_t>::max() >= length ) {
            m_frame.len = 126;
            m_length16 = length;
        }
        else {  // uint64_t
            m_frame.len = 127;
            m_length64 = length;
        }
    }

    inline unsigned int bufLength() const {
        if( 126 == m_frame.len ) {
            return m_length16;
        }
        else if( 127 == m_frame.len ) {
            return m_length64;
        }
        else {
            return m_frame.len;
        }
    }

    void applyMask( char buffer[], const unsigned int bufSize ) const {
        for( unsigned int i = 0; i < bufSize; ++i ) {
            buffer[i] ^= m_sendMaskBytes[i % sizeof( m_sendMaskBytes )];
        }
    }

    bool send( ISocket& sock ) const {
        if( !sock.sendData( m_frame ) ) {
            return false;
        }

        if( 126 == m_frame.len ) {
            const auto nLength = ::htons( m_length16 );
            if( !sock.sendData( nLength ) ) {
                WS_LOGE( "Can't send length16" );
                return false;
            }
        }
        else if( 127 == m_frame.len ) {
            const auto nLength = ::htonl( m_length64 );
            if( !sock.sendData( nLength ) ) {
                WS_LOGE( "Can't send length64" );
                return false;
            }
        }

        if( 1 == m_frame.mask ) {
            m_sendMask32b += 1;
            if( !sock.sendAll( reinterpret_cast<const char*>( &m_sendMask32b ), sizeof( m_sendMask32b ) ) ) {
                WS_LOGE( "Can't send mask" );
                return false;
            }
        }

        const auto bufLen = bufLength();
        if( 1 == m_frame.mask && 0 < bufLen ) {
            applyMask( m_buffer, bufLen );
        }

        if( 0 < bufLen && !sock.sendAll( m_buffer, bufLen ) ) {
            WS_LOGE( "Can't send data" );
            return false;
        }

        return true;
    }
};  // end of class WsFrame

void a3_to_a4( std::array<unsigned char, 4>& a4, std::array<unsigned char, 3>& a3 ) {
    a4[0] = ( a3[0] & 0xfc ) >> 2;
    a4[1] = ( ( a3[0] & 0x03 ) << 4 ) + ( ( a3[1] & 0xf0 ) >> 4 );
    a4[2] = ( ( a3[1] & 0x0f ) << 2 ) + ( ( a3[2] & 0xc0 ) >> 6 );
    a4[3] = ( a3[2] & 0x3f );
}

std::string b64_encode( const std::array<char, 16>& key ) {
    static constexpr char b64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                           "abcdefghijklmnopqrstuvwxyz"
                                           "0123456789+/";
    unsigned int idx = 0;
    std::array<unsigned char, 3> a3;

    std::string encodedStr;
    encodedStr.reserve( 25 );

    for( auto itr = std::rbegin( key ); std::rend( key ) != itr; ++itr ) {
        a3[idx++] = *itr;
        if( idx < 3 ) {
            continue;
        }

        std::array<unsigned char, 4> a4;
        a3_to_a4( a4, a3 );

        for( idx = 0; idx < 4; idx++ ) {
            encodedStr.push_back( b64_alphabet[a4[idx]] );
        }

        idx = 0;
    }

    if( 0 == idx ) {
        return std::move( encodedStr );
    }

    for( auto j = idx; j < a3.size(); j++ ) {
        a3[j] = '\0';
    }

    std::array<unsigned char, 4> a4;
    a3_to_a4( a4, a3 );

    for( auto j = 0; j < idx + 1; j++ ) {
        encodedStr.push_back( b64_alphabet[a4[j]] );
    }

    while( idx++ < 3 ) {
        encodedStr.push_back( '=' );
    }

    return std::move( encodedStr );
}

std::string createHeader( const std::string& url, const std::string& serial ) {
    std::array<char, 16> randomKey;
    {
        std::uniform_int_distribution<char> distr( 32, 127 );
        std::generate(
            std::begin( randomKey ), std::end( randomKey ), [&distr]() { return distr( common::random::generator ); } );
    }

    std::stringstream ss;
    ss << "GET / HTTP/1.1\r\n";
    ss << "Host: " << url << "\r\n";
    ss << "Connection: Upgrade\r\n"
          "Upgrade: websocket\r\n";
    ss << "Origin: " << url << "\r\n";
    ss << "Sec-WebSocket-Version: 13\r\n";
    ss << "User-Agent: ATOM32-" << serial << "\r\n";
    ss << "Sec-WebSocket-Key: " << b64_encode( randomKey ) << "\r\n";
    ss << "\r\n";
    return ss.str();
}

}  // namespace

WebSocket::WebSocket( ISocket::UPtr&& socket, const bool isSendMask )
: ProxySocket( std::move( socket ) )
, m_isSendMask{ isSendMask } {
#ifdef CONFIG_SPIRAM
    m_outBuffer = static_cast<char*>( heap_caps_malloc( OutputBufferMaxSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_32BIT ) );
    m_inBuffer = static_cast<char*>( heap_caps_malloc( InputBufferMaxSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_32BIT ) );
#endif  // CONFIG_SPIRAM
}

WebSocket::~WebSocket() {
    if( isConnected() ) {
        close();
    }
#ifdef CONFIG_SPIRAM
    free( m_outBuffer );
    free( m_inBuffer );
#endif  // CONFIG_SPIRAM
}

bool WebSocket::sendHeader( const std::string& url ) {
    const auto hdr = createHeader( url, core::serial() );
    WS_LOGI( "Header:\n%s", hdr.c_str() );

    return socket().sendAll( hdr.c_str(), hdr.length() );
}

bool WebSocket::connectToHostViaProxySocket( const char* remoteAddress, const uint16_t remotePort ) {
    WS_LOGI( "connectToHostViaProxySocket(address=%s, port=%d)", remoteAddress, remotePort );

    if( !setWriteTimeout( HandshakeSendTimeout ) ) {
        WS_LOGE( "Can't set write timeout" );
        return false;
    }

    if( !sendHeader( remoteAddress ) ) {
        WS_LOGE( "Can't send header to server" );
        return false;
    }

    if( !socket().receiveUntil( "Upgrade: websocket\r\n", HandshakeReceiveTimeout ) ) {
        WS_LOGW( "Can't receive websocket header" );
        return false;
    }

    // Skiping unnecessary headers
    while( socket().receiveUntil( "\r\n", std::chrono::milliseconds( 100 ) ) )
        ;

    if( !setReadTimeout( DefaultReciveTimeout ) ) {
        WS_LOGE( "Can't set receive timeout" );
        return false;
    }

    if( !setWriteTimeout( DefaultSendTimeout ) ) {
        WS_LOGE( "Can't set send timeout" );
        return false;
    }

    m_lastIoTimepoint = ClockType::now();
    return true;
}

bool WebSocket::writeOutBufferToSocket( const bool isFinal ) {
    WS_LOGD( ">>>> send(payload size=%d, isFinal=%d):\n%s", m_outBufferBytes, (int)isFinal, m_outBuffer );

    const bool isFirstFrame = ( 0 == m_sendedToSocket );
    WsFrame frameOut( isFirstFrame, isFinal, m_outBuffer, m_outBufferBytes, m_isSendMask );

    if( !socket().sendData( frameOut ) ) {
        return false;
    }

    if( isFinal ) {
        m_sendedToSocket = 0;
    }
    else {
        m_sendedToSocket += m_outBufferBytes;
    }

    m_outBufferBytes = 0;

    return trySendPingByTimeout();
}

bool WebSocket::trySendPingByTimeout() {
    if( !m_isWaitingPong && ( ReceiveMaxPeriod / 3 ) < ClockType::now() - m_lastIoTimepoint ) {
        if( !sendPing() ) {
            WS_LOGE( "Can't send ping by timeout" );
            return false;
        }
        else {
            WS_LOGD( "sent ping by timeout" );
        }
        m_isWaitingPong = true;
    }

    return true;
}

int WebSocket::send( const char* srcBuffer, const std::size_t srcBufferSize, const int flags ) {
    const bool isFinal = flags & FinalMessage;
    std::size_t bytesProcessed = 0;
    while( true ) {
        auto freeSpaceInBuff = OutputBufferMaxSize - 1 - m_outBufferBytes;
        auto bytesToCopy = std::min<std::size_t>( srcBufferSize - bytesProcessed, freeSpaceInBuff );
        std::memcpy( m_outBuffer + m_outBufferBytes, srcBuffer + bytesProcessed, bytesToCopy );
        bytesProcessed += bytesToCopy;
        m_outBufferBytes += bytesToCopy;
        m_outBuffer[m_outBufferBytes] = '\0';

        if( !isFinal && ( OutputBufferMaxSize - 2 > m_outBufferBytes ) ) {
            return srcBufferSize;
        }

        if( !writeOutBufferToSocket( isFinal ) ) {
            WS_LOGE( "Can't send buffer to socket" );
            return -1;
        }

        if( isFinal ) {
            return srcBufferSize;
        }
    }  // end of infinite loop
}

int WebSocket::receive( char* dstBuffer, const std::size_t dstBifferSize ) {
    if( 0 == dstBifferSize ) {
        return 0;
    }

    std::size_t receivedBytes = 0;
    while( true ) {
        if( 0 < m_inBufferBytes ) {
            auto bytesToCopy = std::min( m_inBufferBytes, dstBifferSize );
            std::memcpy( dstBuffer, m_inBuffer + m_inBufferPos, bytesToCopy );
            m_inBufferPos += bytesToCopy;
            receivedBytes += bytesToCopy;
            m_inBufferBytes -= bytesToCopy;

            if( receivedBytes == dstBifferSize ) {
                return dstBifferSize;
            }
        }
        // Fall througth if m_inBufferBytes == 0

        auto ret = receiveChunk();
        if( 0 > ret ) {
            return -1;
        }
        else if( 0 == ret ) {
            return trySendPingByTimeout() ? receivedBytes : -1;
        }

        WS_LOGD( "<<<< receive(payload size=%d):\n%s", m_inBufferBytes, m_inBuffer );
    }
}

int WebSocket::receiveChunk() {
    m_inBufferPos = 0;

    while( 0 == m_receivedPayloadBytes ) {
        // Receiving frame
        union {
            Frame frame;
            char frameData[sizeof( Frame )];
        };

        {
            const auto bytesReceived = socket().receive( frameData, sizeof( Frame ) );
            if( 0 > bytesReceived ) {
                return -1;
            }
            else if( 0 == bytesReceived ) {
                if( ReceiveMaxPeriod < ClockType::now() - m_lastIoTimepoint ) {
                    WS_LOGE( "Timeout error" );
                    return -1;
                }

                // No data in socket
                return 0;
            }

            if( sizeof( Frame ) > bytesReceived &&
                !socket().receiveAll( frameData + bytesReceived, sizeof( Frame ) - bytesReceived ) ) {
                WS_LOGE( "Can't receive frame completely" );
                return -1;
            }
        }

        // Calculate payload size
        if( 126 == frame.len ) {
            std::uint16_t tempLen;
            if( !socket().receiveAll( (char*)&tempLen, sizeof( tempLen ) ) ) {
                WS_LOGE( "Can't receive uint16 length" );
                return -1;
            }
            m_receivedPayloadBytes = ::ntohs( tempLen );
        }
        else if( 127 == frame.len ) {
            std::uint64_t tempLen;
            if( !socket().receiveAll( (char*)&tempLen, sizeof( tempLen ) ) ) {
                WS_LOGE( "Can't receive uint64_t length" );
                return -1;
            }
            m_receivedPayloadBytes = ::ntohl( tempLen );
        }
        else {
            m_receivedPayloadBytes = frame.len;
        }

        m_isRecvMask = ( 1 == frame.mask );
        if( m_isRecvMask && !socket().receiveAll( (char*)m_recvMask, sizeof( m_recvMask ) ) ) {
            WS_LOGE( "Can't receive input mask" );
            return -1;
        }

        m_lastIoTimepoint = ClockType::now();

        switch( frame.opCode ) {
        case OPCODE_TEXT:
        case OPCODE_CONTINUE:
            // Go to the beginning of the cycle to read data to destination buffer
            break;

        case OPCODE_PING: {
            WS_LOGD( "Receive OPCODE_PING" );
            if( m_receivedPayloadBytes ) {
                char dummyData[m_receivedPayloadBytes];
                if( !socket().receiveAll( dummyData, m_receivedPayloadBytes ) ) {
                    WS_LOGE( "Can't receive OPCODE_PONG with app data %lld bytes", m_receivedPayloadBytes );
                    return -1;
                }
                else {
                    WsFrame frameOut( dummyData, m_receivedPayloadBytes, m_isSendMask );
                    if( !socket().sendData( frameOut ) ) {
                        WS_LOGE( "Can't send OPCODE_PONG with app data" );
                        return -1;
                    }
                }
                m_receivedPayloadBytes = 0;
                WS_LOGD( "Send OPCODE_PONG  with app data size=%lld", m_receivedPayloadBytes );
            }
            else {
                WsFrame frameOut( nullptr, 0, m_isSendMask );
                if( socket().sendData( frameOut ) ) {
                    WS_LOGD( "Send OPCODE_PONG" );
                    continue;
                }
                else {
                    WS_LOGE( "Can't send OPCODE_PONG without app data" );
                    return -1;
                }
            }
            continue;
        }  // end of case OPCODE_PING

        case OPCODE_PONG: {
            WS_LOGD( "Receive OPCODE_PONG" );
            m_isWaitingPong = false;
            continue;
        }

        // If the WebSocket operation code is close then we are closing the connection.
        case OPCODE_CLOSE: {
            WS_LOGD( "Receive OPCODE_CLOSE" );
            return -1;
        }

        case OPCODE_BINARY: {
            WS_LOGE( "Receive opcode OPCODE_BINARY, not supported" );
            return -1;
        }

        default: {
            WS_LOGE( "Receive unknown opcode: %d", frame.opCode );
            return -1;
        }
        }  // end of switch
    }      // end if ( 0 == m_remainingBytes )

    // Uploading data to m_inBuffer
    auto maxDataSizeToRead = std::min<size_t>( InputBufferMaxSize - 1, m_receivedPayloadBytes - m_readedPayloadBytes );
    auto receivedDataSize = socket().receive( m_inBuffer, maxDataSizeToRead );
    if( 0 > receivedDataSize ) {
        WS_LOGE( "Can't receive data" );
        return -1;
    }
    else if( 0 == receivedDataSize ) {
        // No payload data in socket
        return 0;
    }

    if( m_isRecvMask ) {
        for( auto i = m_readedPayloadBytes; i < m_readedPayloadBytes + receivedDataSize; ++i ) {
            m_inBuffer[i] ^= m_recvMask[i % 4];
        }
    }

    m_readedPayloadBytes += receivedDataSize;
    if( m_readedPayloadBytes == m_receivedPayloadBytes ) {
        m_receivedPayloadBytes = 0;
        m_readedPayloadBytes = 0;
    }

    m_inBufferBytes = receivedDataSize;
    m_inBuffer[m_inBufferBytes] = '\0';

    return receivedDataSize;
}

bool WebSocket::close() {
    WS_LOGI( "Close web socket" );

    bool isSuccess = true;
    if( socket().isConnected() && isConnected() ) {
        WS_LOGD( "Send OPCODE_PING" );
        Frame frame = {};
        frame.fin = 1;
        frame.opCode = OPCODE_CLOSE;
        if( !socket().sendAll( reinterpret_cast<char*>( &frame ), sizeof( Frame ) ) ) {
            isSuccess = false;
        }
        core::this_thread::yield();
    }

    reset();

    if( !ProxySocket::close() ) {
        isSuccess = false;
    }

    return isSuccess;
}

void WebSocket::reset() {
    m_outBufferBytes = 0;
    m_sendedToSocket = 0;
    m_receivedPayloadBytes = 0;
    m_readedPayloadBytes = 0;
    m_isRecvMask = false;
}

bool WebSocket::sendPing() {
    WS_LOGD( "Send OPCODE_PING" );
    Frame frame = {};
    frame.fin = 1;
    frame.opCode = OPCODE_PING;

    const bool isSuccess = socket().sendAll( reinterpret_cast<char*>( &frame ), sizeof( Frame ) );
    return isSuccess;
}

}  // namespace network
