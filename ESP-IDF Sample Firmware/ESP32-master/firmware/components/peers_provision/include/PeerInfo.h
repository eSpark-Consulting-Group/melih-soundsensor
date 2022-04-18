#pragma once

#include "IWriteStream.h"
#include "IReadStream.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "DebugUtils.h"

namespace provision {
namespace peers {

class PeerInfo {
public:
    PeerInfo() = default;

    bool read( common::IReadStream& stream ) {
        return stream.read( m_id ) && stream.read( m_serial ) && stream.read( m_ip ) && stream.read( m_status ) &&
               stream.read( m_tokens );
    }

    bool write( common::IWriteStream& stream ) const {
        return stream.write( m_id ) && stream.write( m_serial ) && stream.write( m_ip ) && stream.write( m_status ) &&
               stream.write( m_tokens );
    }

    inline std::string id() const {
        return m_id;
    }

    inline void setId( const std::string& id ) {
        if( id != m_id ) {
            m_id = id;
        }
    }

    inline std::string serial() const {
        return m_serial;
    }

    inline void setSerial( const std::string& serial ) {
        if( m_serial != serial ) {
            m_serial = serial;
        }
    }

    inline std::list<std::string> ip() const {
        return m_ip;
    };

    void setIp( const std::list<std::string>& ip ) {
        for( auto i : ip ) {
            FUNC_LOGE( "ip: %s", i.c_str() );
        }

        FUNC_LOGE( "m_ip = ip;" );

        m_ip = ip;
    }

    inline std::list<std::string> tokens() const {
        return m_ip;
    };

    inline void setTokens( const std::list<std::string>& tokens ) {
        m_tokens = tokens;
    }

    inline std::string status() const {
        return m_status;
    }

    inline void setStatus( const std::string& status ) {
        if( m_status != status ) {
            m_status = status;
        }
    }

private:
    std::string m_id;
    std::string m_serial;
    std::list<std::string> m_ip;
    std::list<std::string> m_tokens;
    std::string m_status;
};

};  // namespace peers
};  // namespace provision
