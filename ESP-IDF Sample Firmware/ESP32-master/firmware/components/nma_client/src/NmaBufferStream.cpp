
#include <NmaBufferStream.h>

#include <string.h>
#include <algorithm>

#include <DebugUtils.h>

namespace nma {

BufferStream::BufferStream( size_t BufferLength )
: m_sendingBuffer( MinBufferLength ) {}

int BufferStream::write( const char* buffer, size_t size ) {
    if( ( size == 0 ) || ( buffer == nullptr ) ) {
        return 0;
    }

    size_t freeSize = m_sendingBuffer.size() - m_writtenBytes;
    if( size > freeSize ) {
        m_sendingBuffer.resize( m_sendingBuffer.size() + std::max( (size_t)MinBufferLength, size - freeSize ) );
    }
    strncpy( m_sendingBuffer.data() + m_writtenBytes, buffer, size );
    m_writtenBytes += size;
    return size;
}

bool BufferStream::close() {
    reset();

    return true;
}

}  // namespace nma
