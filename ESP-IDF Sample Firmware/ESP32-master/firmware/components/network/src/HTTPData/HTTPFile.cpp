#include <HTTPData/HTTPFile.h>
#include <DebugUtils.h>

#include <Storage.h>
#include <StorageFile.h>

namespace network {

HTTPFile::HTTPFile( const std::string& filename )
: m_filename( filename ) {
    m_buffer.reserve( BUFFER_SIZE );
}

HTTPFile::~HTTPFile() {
    flushBuffer();
}

void HTTPFile::setDataLen( size_t len ) {}

void HTTPFile::setDataType( const char* type ) {}

void HTTPFile::setIsChunked( bool chunked ) {}

network::HTTPDataResult HTTPFile::write( const char* buf, size_t len ) {
    using namespace network;
    if( pushToBuffer( buf, len ) ) {
        m_writtenByteCount += len;
    }
    else {
        if( !storage::Storage::get().remove( m_filename.c_str() ) ) {
            FUNC_LOGE( "Fail removing plugin file: %s", m_filename.c_str() );
        }
        return HTTPDataResult::Error;
    }
    return HTTPDataResult::OK;
}

void HTTPFile::writeReset() {}

uint32_t HTTPFile::getWrittenByteCount() const {
    return m_writtenByteCount;
}

bool HTTPFile::pushToBuffer( const char* data, size_t len ) {
    if( m_buffer.size() + len > BUFFER_SIZE ) {
        if( !flushBuffer() ) {
            return false;
        }
    }
    if( len < BUFFER_SIZE ) {
        std::copy( data, data + len, std::back_inserter( m_buffer ) );
    }
    else {
        if( !writeToFile( data, len ) ) {
            return false;
        }
    }
    return true;
}

bool HTTPFile::flushBuffer() {
    if( m_buffer.empty() ) {
        return true;
    }
    if( !writeToFile( m_buffer.data(), m_buffer.size() ) ) {
        return false;
    }
    m_buffer.clear();
    return true;
}

bool HTTPFile::writeToFile( const char* data, size_t len ) {
    storage::File plugin;
    if( !( plugin.open( m_filename.c_str(), "a", std::chrono::milliseconds( 100 ), false ) &&
           plugin.seek( 0, SEEK_END ) ) ) {
        FUNC_LOGE( "Failed opening file" );
        return false;
    }
    if( !plugin.write( data, len ) ) {
        FUNC_LOGE( "Failed writing to file" );
        return false;
    }
    return true;
}

}  // namespace network
