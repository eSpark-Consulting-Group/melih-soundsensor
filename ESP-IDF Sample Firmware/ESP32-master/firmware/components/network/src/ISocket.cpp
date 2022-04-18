#include "ISocket.h"

#include "DebugUtils.h"

namespace network {

const SystemTimeval ISocket::readAllTimeout( 300, 0 );

bool ISocket::sendAll( const char* buffer, const unsigned int bufSize ) {
    const auto savedTimeout = getWriteTimeval();

    if( bufSize < 0x4000 ) {
        if( !setWriteTimeout( SystemTimeval( 16, 0 ) ) ) {
            return false;
        }
    }
    else if( bufSize < 0x8000 ) {
        if( !setWriteTimeout( SystemTimeval( 32, 0 ) ) ) {
            return false;
        }
    }
    else if( !setWriteTimeout( SystemTimeval( 64, 0 ) ) ) {
        return false;
    }

    if( send( buffer, bufSize ) != bufSize ) {
        return false;
    }

    return setWriteTimeout( savedTimeout );
}

bool ISocket::receiveAll( char* buffer, const unsigned int bufSize ) {
    const auto savedTimeout = getReadTimeval();
    if( !setReadTimeout( readAllTimeout ) ) {
        return false;
    }

    if( receive( buffer, bufSize ) != bufSize ) {
        return false;
    }

    return setReadTimeout( savedTimeout );
}

bool ISocket::receiveUntil( const std::string& tmp, const std::chrono::milliseconds& timeout ) {
    if( tmp.empty() ) {
        return true;
    }

    const auto prevTimeout = getReadTimeval();
    if( !setReadTimeout( timeout ) ) {
        return false;
    }

    const auto endItr = tmp.end();
    auto itr = tmp.begin();
    char ch;
    while( true ) {
        if( 1 != receive( &ch, 1 ) ) {
            setReadTimeout( prevTimeout );
            return false;
        }

        if( *itr == ch ) {
            if( endItr == ++itr ) {
                return setReadTimeout( prevTimeout );
            }
        }
        else {
            itr = tmp.begin();
        }
    }
}

bool ISocket::skip( const std::chrono::milliseconds& timeout ) {
    const auto prevTimeout = getReadTimeval();
    if( !setReadTimeout( timeout ) ) {
        return false;
    }

    char ch;
    while( 1 == receive( &ch, 1 ) ) {
        // Nothing to do
    }

    return setReadTimeout( prevTimeout );
}

bool ProxySocket::connectToHost( const char* remoteAddress, const uint16_t remotePort ) {
    if( isConnected() ) {
        close();
    }

    if( !isValid() || !socket().connectToHost( remoteAddress, remotePort ) ) {
        return false;
    }

    if( !connectToHostViaProxySocket( remoteAddress, remotePort ) ) {
        socket().close();
        return false;
    }

    setState( States::ConnectedState );
    return true;
}

}  // namespace network
