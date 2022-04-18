#pragma once

#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "DebugUtils.h"

using namespace common::resources;

namespace core {

class ControllerStorageToken {
public:
    ControllerStorageToken() {
        load();
    };

    bool load();
    bool save();

    inline std::string tokenString() const {
        return m_tokenString;
    }

    inline void setTokenString( const std::string& tokenString ) {
        m_tokenString = tokenString;
    }

    inline int expireTs() const {
        return m_expiresTs;
    }

    inline void setExpireTs( const int expiresTs ) {
        m_expiresTs = expiresTs;
    }

    inline std::string xapp() const {
        return m_xapp;
    }

    inline void setXapp( const std::string& xapp ) {
        m_xapp = xapp;
    }

    inline std::list<std::string> uploadEndpoints() const {
        return m_uploadEndpoints;
    }

    inline void setUploadEndpoints( const std::list<std::string>& uploadEndpoints ) {
        m_uploadEndpoints = uploadEndpoints;
    }

    inline std::list<std::string> downloadEndpoints() const {
        return m_downloadEndpoints;
    }

    inline void setDownloadEndpoints( const std::list<std::string>& downloadEndpoints ) {
        m_downloadEndpoints = downloadEndpoints;
    }

private:
    bool read( common::IReadStream& stream );
    bool write( common::IWriteStream& stream ) const;

private:
    std::string m_tokenString;
    int m_expiresTs;
    std::string m_xapp;
    std::list<std::string> m_uploadEndpoints;
    std::list<std::string> m_downloadEndpoints;
};

}  // namespace core
