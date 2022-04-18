/* HTTPText.cpp */
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

#include <HTTPData/HTTPText.h>

#include <cstring>
#include <algorithm>

namespace network {

using std::memcpy;
using std::strlen;
using std::strncpy;

HTTPText::HTTPText( char* str )
: m_str( str )
, m_size( strlen( str ) + 1 )
, m_pos( 0 ) {}

HTTPText::HTTPText( char* str, size_t size )
: m_str( str )
, m_size( size )
, m_pos( 0 ) {}

// IHTTPDataIn
void HTTPText::readReset() {
    m_pos = 0;
}

HTTPDataResult HTTPText::read( char* buf, size_t len, size_t* pReadLen ) {
    *pReadLen = std::min( len, m_size - 1 - m_pos );
    memcpy( buf, m_str + m_pos, *pReadLen );
    m_pos += *pReadLen;
    return HTTPDataResult::OK;
}

HTTPDataResult HTTPText::getDataType( char* type, size_t maxTypeLen )  // Internet media type for Content-Type header
{
    strncpy( type, "text/plain", maxTypeLen - 1 );
    type[maxTypeLen - 1] = '\0';
    return HTTPDataResult::OK;
}

bool HTTPText::getIsChunked()  // For Transfer-Encoding header
{
    return false;
}

size_t HTTPText::getDataLen()  // For Content-Length header
{
    return m_size - 1;
}

// IHTTPDataOut
void HTTPText::writeReset() {
    m_pos = 0;
}

HTTPDataResult HTTPText::write( const char* buf, size_t len ) {
    size_t writeLen = std::min( len, m_size - 1 - m_pos );
    memcpy( m_str + m_pos, buf, writeLen );
    m_pos += writeLen;
    m_str[m_pos] = '\0';
    return HTTPDataResult::OK;
}

void HTTPText::setDataType( const char* type )  // Internet media type from Content-Type header
{}

void HTTPText::setIsChunked( bool chunked )  // From Transfer-Encoding header
{}

void HTTPText::setDataLen( size_t len ) {}

}  // namespace network
