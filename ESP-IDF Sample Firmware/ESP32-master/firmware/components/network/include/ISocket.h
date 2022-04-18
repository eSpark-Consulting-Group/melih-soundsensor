#pragma once

#include "IWriteStream.h"
#include "IReadStream.h"

#include <cstdint>
#include <memory>
#include <string>
#include <chrono>

namespace network {

class SystemTimeval : public ::timeval {
public:
    using SecType = decltype(::timeval::tv_sec );
    using UsecType = decltype(::timeval::tv_usec );

    SystemTimeval()
    : ::timeval( {} ) {}

    SystemTimeval( const ::timeval& tval )
    : ::timeval( tval ) {}

    SystemTimeval( const SecType& sec, const UsecType& usec )
    : ::timeval( { sec, usec } ) {}

    template <typename _Rep, typename _Period>
    SystemTimeval( const std::chrono::duration<_Rep, _Period>& time ) {
        const auto sec = std::chrono::duration_cast<std::chrono::seconds>( time );
        const auto usec = std::chrono::duration_cast<std::chrono::microseconds>( time - sec );
        tv_sec = sec.count();
        tv_usec = usec.count();
    }

    const auto& getSec() const {
        return tv_sec;
    }

    const auto& getUsec() const {
        return tv_usec;
    }

    SystemTimeval& operator=( const ::timeval& tval ) {
        this->tv_sec = tval.tv_sec;
        this->tv_usec = tval.tv_usec;
        return *this;
    }

    template <typename _Rep, typename _Period>
    std::chrono::duration<_Rep, _Period> toChronoDuration() const {
        std::chrono::duration<_Rep, _Period> val;
        val += std::chrono::seconds( tv_sec );
        val += std::chrono::microseconds( tv_usec );
        return val;
    }
};

inline bool operator==( const ::timeval& timeout1, const ::timeval& timeout2 ) {
    return timeout1.tv_sec == timeout2.tv_sec && timeout1.tv_usec == timeout2.tv_usec;
}

class ISocket {
public:
    enum class States { UnconnectedState, ConnectedState, BoundState };
    using UPtr = std::unique_ptr<ISocket>;

    static const SystemTimeval readAllTimeout;

    virtual ~ISocket() = default;

    virtual bool connectToHost( const char* remoteAddress, const uint16_t remotePort ) = 0;

    inline bool connectToHost( const std::string& remoteAddress, const uint16_t remotePort ) {
        return connectToHost( remoteAddress.c_str(), remotePort );
    }

    virtual int send( const char* srcBuffer, const std::size_t srcBufferSize, const int flags = 0 ) = 0;

    virtual int receive( char* dstBuffer, const std::size_t dstBufferSize ) = 0;

    virtual bool setReadTimeout( const SystemTimeval& timeout ) = 0;

    template <typename _Rep, typename _Period>
    std::chrono::duration<_Rep, _Period> getReadTimeout() const {
        return getReadTimeval().toChronoDuration<_Rep, _Period>();
    }

    virtual SystemTimeval getReadTimeval() const = 0;

    virtual bool setWriteTimeout( const SystemTimeval& timeout ) = 0;

    template <typename _Rep, typename _Period>
    std::chrono::duration<_Rep, _Period> getWriteTimeout() const {
        return getWriteTimeval().toChronoDuration<_Rep, _Period>();
    }

    virtual SystemTimeval getWriteTimeval() const = 0;

    virtual bool close() = 0;

    virtual bool flush() {
        return true;
    }

    virtual bool setBlocking( bool isBlockingMode ) = 0;

public:
    inline States state() const {
        return m_state;
    }

    inline bool isConnected() const {
        return States::ConnectedState == state() || States::BoundState == state();
    }

    inline operator bool() const {
        return isConnected();
    }

public:
    bool sendAll( const char* buffer, const unsigned int bufSize );

    bool receiveAll( char* buffer, const unsigned int bufSize );

    bool receiveUntil( const std::string& tmp,
                       const std::chrono::milliseconds& timeout = std::chrono::milliseconds( 1000 ) );

    bool skip( const std::chrono::milliseconds& timeout = std::chrono::milliseconds( 1000 ) );

public:
    template <typename ValueType,
              typename std::enable_if<std::is_integral<ValueType>::value || std::is_floating_point<ValueType>::value ||
                                          std::is_enum<ValueType>::value,
                                      int>::type = 0>
    inline bool receiveData( ValueType& value ) {
        return receiveAll( reinterpret_cast<char*>( &value ), sizeof( ValueType ) );
    }

    template <typename T, typename std::enable_if<std::is_class<T>::value, int>::type = 0>
    inline bool receiveData( T& object ) {
        return object.receive( *this );
    }

public:
    template <typename ValueType,
              typename std::enable_if<std::is_integral<ValueType>::value || std::is_floating_point<ValueType>::value ||
                                          std::is_enum<ValueType>::value,
                                      int>::type = 0>
    inline bool sendData( const ValueType& value ) {
        return sendAll( reinterpret_cast<const char*>( &value ), sizeof( ValueType ) );
    }

    template <typename T, typename std::enable_if<std::is_class<T>::value, int>::type = 0>
    inline bool sendData( const T& object ) {
        return object.send( *this );
    }

protected:
    inline void setState( const States state ) {
        m_state = state;
    }

private:
    States m_state = States::UnconnectedState;
};  // class ISocket

class ProxySocket : public ISocket {
public:
    ProxySocket( ISocket::UPtr&& socket )
    : m_socket( std::move( socket ) ) {}

    ~ProxySocket() {
        if( isConnected() ) {
            close();
        }
    }

    inline operator bool() const {
        return m_socket.operator bool();
    }

    inline bool isValid() const {
        return m_socket.operator bool();
    }

    inline const auto& socket() const {
        return *m_socket;
    }

    inline auto& socket() {
        return *m_socket;
    }

public:  // ISocket
    bool connectToHost( const char* remoteAddress, const uint16_t remotePort ) override;

    using ISocket::connectToHost;

    int send( const char* srcBuffer, const std::size_t srcBufferSize, const int flags = 0 ) override {
        return isValid() ? socket().send( srcBuffer, srcBufferSize, flags ) : -1;
    }

    int receive( char* dstBuffer, const std::size_t dstBufferSize ) override {
        return isValid() ? socket().receive( dstBuffer, dstBufferSize ) : -1;
    }

    bool setReadTimeout( const SystemTimeval& timeout ) override {
        return isValid() && socket().setReadTimeout( timeout );
    }

    using ISocket::setReadTimeout;

    SystemTimeval getReadTimeval() const override {
        return isValid() ? socket().getReadTimeval() : SystemTimeval();
    }

    bool setWriteTimeout( const SystemTimeval& timeout ) override {
        return isValid() && socket().setWriteTimeout( timeout );
    }

    using ISocket::setWriteTimeout;

    SystemTimeval getWriteTimeval() const override {
        return isValid() ? socket().getWriteTimeval() : SystemTimeval();
    }

    bool close() override {
        setState( States::UnconnectedState );
        if( !isValid() || !socket().isConnected() ) {
            return true;
        }
        return socket().close();
    }

    bool flush() override {
        return isValid() && socket().flush();
    }

    bool setBlocking( bool isBlockingMode ) override {
        return isValid() && socket().setBlocking( isBlockingMode );
    }

private:
    virtual bool connectToHostViaProxySocket( const char* remoteAddress, const uint16_t remotePort ) {
        return true;
    }

private:
    ISocket::UPtr m_socket;
};  // class ProxySocket

class WSockStream : public common::IWriteStream {
public:
    WSockStream( ISocket& sock )
    : m_sock( sock ) {}

    int write( const char* buffer, std::size_t bufSize ) override {
        return m_sock.send( buffer, bufSize );
    }

    bool flush() override {
        return m_sock.flush();
    }

    virtual bool close() {
        return m_sock.close();
    }

protected:
    ISocket& m_sock;
};  // class WSockStream

class RSockStream : public common::IReadStream {
public:
    RSockStream( ISocket& sock )
    : m_sock( sock ) {}

    int read( char* data, size_t dataSize ) override {
        return m_sock.receive( data, dataSize );
    }

    bool reset( unsigned int offset ) override {
        return m_sock.flush();
    }

private:
    ISocket& m_sock;
};  // class RSockStream

}  // namespace network
