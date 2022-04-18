/* HTTPJson.cpp */
/* Copyright (C) 2012 mbed.org, MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <HTTPData/HTTPJson.h>

#include <algorithm>

namespace network {

namespace {
constexpr char JsonFormatString[] = "application/json";
}

using std::strncpy;

HTTPJson::HTTPJson( char* json_str )
: HTTPText( json_str ) {}

HTTPJson::HTTPJson( char* json_str, size_t size )
: HTTPText( json_str, size ) {}

HTTPDataResult HTTPJson::getDataType( char* type,
                                      size_t maxTypeLen )  // Internet media type for Content-Type header
{
    const auto minSize = std::min( maxTypeLen - 1, sizeof( JsonFormatString ) );
    strncpy( type, JsonFormatString, minSize );
    type[minSize - 1] = '\0';
    return HTTPDataResult::OK;
}

}  // namespace network
