#pragma once

#include "ISocket.h"
#include <string>
#include <memory>

namespace network {

struct IServer {
    using StringType = std::string;
    using UPtr = std::unique_ptr<IServer>;

    virtual ~IServer() = default;

    virtual bool listen( const StringType locAddr = StringType(), const std::uint16_t locPort = 0 ) = 0;

    bool listen( const char* address = nullptr, const std::uint16_t port = 0 ) {
        return nullptr == address ? listen( StringType(), port ) : listen( StringType( address ), port );
    }

    virtual ISocket::UPtr nextPendingConnection() = 0;

    virtual bool isListening() const = 0;

    virtual bool close() = 0;
};

}  // namespace network
