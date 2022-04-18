
#include <HTTPData/HTTPXForm.h>

namespace network {

using std::strncpy;

HTTPXForm::HTTPXForm( char* url )
: HTTPText( url ) {}

HTTPXForm::HTTPXForm( char* url, size_t size )
: HTTPText( url, size ) {}

HTTPDataResult HTTPXForm::getDataType( char* type, size_t maxTypeLen )  // Internet media type for Content-Type header
{
    strncpy( type, "application/x-www-form-urlencoded", maxTypeLen - 1 );
    type[maxTypeLen - 1] = '\0';
    return HTTPDataResult::OK;
}

}  // namespace network
