
#pragma once

#include <IHTTPData.h>

namespace common {
class JsonStreamParser;
}

namespace network {

class HTTPJsonParsingData final : public IHTTPDataIn {
public:
    HTTPJsonParsingData( common::JsonStreamParser& jsonParser );

public:
    void writeReset() override;

    HTTPDataResult write( const char* buf, size_t len ) override;

    void setDataType( const char* /*type*/ ) override;

    void setIsChunked( bool /*chunked*/ ) override;

    void setDataLen( size_t /*len*/ ) override;

private:
    common::JsonStreamParser& m_jsonParser;
};

}  // namespace network
