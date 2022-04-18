
#pragma once

#include <HTTPData/HTTPText.h>

namespace network {

class HTTPXForm final : public HTTPText {
public:
    HTTPXForm( char* url );

    HTTPXForm( char* url, size_t size );

protected:
    HTTPDataResult getDataType( char* type, size_t maxTypeLen ) override;
};

}  // namespace network
