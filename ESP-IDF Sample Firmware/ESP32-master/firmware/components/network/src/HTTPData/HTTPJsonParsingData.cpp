
#include <HTTPData/HTTPJsonParsingData.h>

#include <JsonStreamParser.h>

namespace network {

HTTPJsonParsingData::HTTPJsonParsingData( common::JsonStreamParser& jsonParser )
: m_jsonParser( jsonParser ) {}

void HTTPJsonParsingData::writeReset() {}

HTTPDataResult HTTPJsonParsingData::write( const char* buf, size_t len ) {
    return m_jsonParser.parse( buf, len ) ? HTTPDataResult::OK : HTTPDataResult::Error;
}

void HTTPJsonParsingData::setDataType( const char* /*type*/ ) {}

void HTTPJsonParsingData::setIsChunked( bool /*chunked*/ ) {}

void HTTPJsonParsingData::setDataLen( size_t /*len*/ ) {}

}  // namespace network
