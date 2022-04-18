#pragma once

#include "ISocket.h"
#include "IReadStream.h"
#include "IWriteStream.h"

#include <string>
#include <cstdint>
#include <chrono>
#include <array>

namespace network {

class UWebSocket : public ProxySocket {
public:
    using UPtr = std::unique_ptr<UWebSocket>;
    using ClockType = std::chrono::steady_clock;

    enum : int {
        Default = 0,
        FinalMessage = 1 << 1,
    };

    //#ifdef CONFIG_SPIRAM
    static constexpr size_t OutputBufferMaxSize = 8 * 1024;
    static constexpr size_t InputBufferMaxSize = 8 * 1024;

    UWebSocket( ISocket::UPtr&& socket );

    ~UWebSocket();

public:  // ISocket
    using ProxySocket::connectToHost;

    int send( const char* srcBuffer, const std::size_t srcBufferSize, const int flags = 0 ) override;

    int receive( char* dstBuffer, const std::size_t dstBufferSize ) override;

    bool close() override;

private:
    bool connectToHostViaProxySocket( const char* remoteAddress, const uint16_t remotePort ) override;

    bool executeHandshake();

    std::string createHeader( const std::string& url, const std::string& serial );

    bool writeOutBufferToSocket( bool isFinal );

    int receiveChunk( char* dstBuffer, const std::size_t dstBufferSize );

    void reset();

private:
    ClockType::time_point m_lastIoTimepoint;

    char m_inBuffer[InputBufferMaxSize];

    std::size_t m_inBufferBytes = 0;
    std::size_t m_inBufferPos = 0;

    std::size_t m_remainingBytes = 0;
    uint8_t m_mask[4];
    bool m_isMask = false;

    bool FW_update();
    char m_outBuffer[OutputBufferMaxSize];

    std::uint16_t m_outBufferBytes = 0;
    std::size_t m_sendedToSocket = 0;
};

}  // namespace network
