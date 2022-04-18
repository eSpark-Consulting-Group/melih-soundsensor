#pragma once

#include "ISocket.h"
#include "IReadStream.h"
#include "IWriteStream.h"

#include <string>
#include <cstdint>
#include <chrono>
#include <array>

namespace network {

class WebSocket : public ProxySocket {
public:
    using UPtr = std::unique_ptr<WebSocket>;
    using ClockType = std::chrono::steady_clock;

    enum {
        Default = 0,
        FinalMessage = 1 << 1,
    };

#ifdef CONFIG_SPIRAM
    static constexpr size_t OutputBufferMaxSize = 8 * 1024;
    static constexpr size_t InputBufferMaxSize = 8 * 1024;
#else   // not CONFIG_SPIRAM
    static constexpr size_t OutputBufferMaxSize = 512;
    static constexpr size_t InputBufferMaxSize = 256;
#endif  // CONFIG_SPIRAM

    WebSocket( ISocket::UPtr&& socket, const bool isSendMask = true );

    ~WebSocket();

public:  // ISocket
    using ProxySocket::connectToHost;

    int send( const char* srcBuffer, const std::size_t srcBufferSize, const int flags = 0 ) override;

    int receive( char* dstBuffer, const std::size_t dstBufferSize ) override;

    bool close() override;

private:
    bool connectToHostViaProxySocket( const char* remoteAddress, const uint16_t remotePort ) override;

    bool executeHandshake();

    bool writeOutBufferToSocket( const bool isFinal );

    int receiveChunk();

    void reset();

    bool sendPing();

    bool trySendPingByTimeout();

    bool sendHeader( const std::string& url );

private:
    ClockType::time_point m_lastIoTimepoint;
    bool m_isWaitingPong = false;

#ifdef CONFIG_SPIRAM
    char* m_inBuffer;
#else   // not CONFIG_SPIRAM
    char m_inBuffer[InputBufferMaxSize];
#endif  // CONFIG_SPIRAM
    std::size_t m_inBufferBytes = 0;
    std::size_t m_inBufferPos = 0;
    std::uint64_t m_receivedPayloadBytes = 0;
    std::size_t m_readedPayloadBytes = 0;
    uint8_t m_recvMask[4];
    bool m_isRecvMask = false;

#ifdef CONFIG_SPIRAM
    char* m_outBuffer;
#else   // not CONFIG_SPIRAM
    char m_outBuffer[OutputBufferMaxSize];
#endif  // CONFIG_SPIRAM
    std::size_t m_outBufferBytes = 0;
    std::size_t m_sendedToSocket = 0;

    bool m_isSendMask;
};

}  // namespace network
