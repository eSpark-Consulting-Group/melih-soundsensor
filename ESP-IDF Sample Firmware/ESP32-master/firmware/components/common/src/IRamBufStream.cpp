#include <IRamBufStream.h>
#include <esp_heap_caps.h>
#include <algorithm>

//#define ENABLE_IBS_LOGS
#ifdef ENABLE_IBS_LOGS

#include <esp_log.h>
#include "EspLogTags.h"

#define IBS_LOGI( ... ) ESP_LOGI(::esp_log_tags::RAM_BUFFER_STREAM, __VA_ARGS__ )
#define IBS_LOGE( ... ) ESP_LOGE(::esp_log_tags::RAM_BUFFER_STREAM, __VA_ARGS__ )
#define IBS_LOGW( ... ) ESP_LOGW(::esp_log_tags::RAM_BUFFER_STREAM, __VA_ARGS__ )

#else

#define IBS_LOGI( ... ) \
    do {                \
    } while( false )
#define IBS_LOGE( ... ) \
    do {                \
    } while( false )
#define IBS_LOGW( ... ) \
    do {                \
    } while( false )

#endif

namespace common {
template <typename Data, typename Clbk>
size_t IRamBufferStream::WrapBufOps( Data data, size_t dataSize, size_t& _ctr, Clbk _clbk ) {
    if( _ctr + dataSize >= m_bufsize ) {
        size_t bytes_to_write = m_bufsize - _ctr;
        ( this->*_clbk )( _ctr, data, bytes_to_write );
        _ctr = 0;
        ( this->*_clbk )( _ctr, data + bytes_to_write, dataSize - bytes_to_write );
        _ctr = dataSize - bytes_to_write;
    }
    else {
        ( this->*_clbk )( _ctr, data, dataSize );
        _ctr += dataSize;
    }

    return dataSize;
}

IRamBufferStream::IRamBufferStream( const size_t _buf_len )
: m_bufsize( ( _buf_len + sizeof( ElemType ) ) / sizeof( ElemType ) * sizeof( ElemType ) ) {
    m_buf = static_cast<ElemTypePtr>( heap_caps_malloc( m_bufsize, MALLOC_CAP_32BIT ) );
    IBS_LOGI( "Create BufferStream instance requested size = %zi, allocated size = %zi\n", _buf_len, m_bufsize );
}

IRamBufferStream::~IRamBufferStream() {
    heap_caps_free( m_buf );
    IBS_LOGI( "BufferStream instance was deleted\n" );
}

void IRamBufferStream::writeChunk( size_t pos, const char* data, size_t dataSize ) {
    IBS_LOGI( "BufferStream::write(pos=%zi, dataSize=%zi)\n", pos, dataSize );

    auto start_epos = pos / ElemSize;
    auto start_bpos = pos % ElemSize;
    if( start_bpos > 0 ) {
        union {
            ElemType elem;
            char b[ElemSize];
        };
        elem = m_buf[start_epos];

        auto bytes_to_copy = std::min( ElemSize - start_bpos, dataSize );
        std::copy( data, data + bytes_to_copy, b + start_bpos );
        data += bytes_to_copy;
        m_buf[start_epos] = elem;

        if( bytes_to_copy == dataSize )
            return;
        start_epos += 1;
    }

    auto end = pos + dataSize;
    auto end_epos = end / ElemSize;

    {
        ElemTypePtr pstart = ( const ElemTypePtr )( data );
        const ElemTypePtr pend = pstart + end_epos - start_epos;
        // std::copy( pstart, pend, m_buf + start_epos );
        for( ElemTypePtr bptr = m_buf + start_epos; pstart < pend; ++pstart, ++bptr )
            *bptr = *pstart;
        data = (DataPtr)pend;
    }

    auto end_bpos = end % ElemSize;
    if( end_bpos > 0 ) {
        union {
            ElemType elem;
            char b[ElemSize];
        };
        elem = m_buf[end_epos];
        std::copy( data, data + end_bpos, b );
        m_buf[end_epos] = elem;
    }
}

int IRamBufferStream::write( const char* data, size_t dataSize ) {
    size_t total_bytes_to_write = std::min( free(), dataSize );
    return WrapBufOps( data, total_bytes_to_write, m_wpos, &IRamBufferStream::writeChunk );
}

void IRamBufferStream::readChunk( size_t pos, char* data, size_t dataSize ) {
    IBS_LOGI( "BufferStream::read(pos=%i, dataSize=%i)\n", pos, dataSize );

    auto start_epos = pos / ElemSize;
    auto start_bpos = pos % ElemSize;
    if( start_bpos > 0 ) {
        union {
            ElemType elem;
            char b[ElemSize];
        };
        elem = m_buf[start_epos];
        auto bytes_to_copy = std::min( ElemSize - start_bpos, dataSize );
        std::copy( b + start_bpos, b + start_bpos + bytes_to_copy, data );
        data += bytes_to_copy;

        if( dataSize == bytes_to_copy )
            return;
        start_epos += 1;
    }

    auto end = pos + dataSize;
    auto end_epos = end / ElemSize;

    {
        ElemTypePtr pstart = ( const ElemTypePtr )( data );
        const ElemTypePtr pend = pstart + end_epos - start_epos;
        ElemTypePtr bend = m_buf + end_epos;
        for( ElemTypePtr bstart = m_buf + start_epos; bstart < bend; ++bstart, ++pstart )
            *pstart = *bstart;
        // std::copy(m_buf + start_epos, m_buf + end_epos, pstart);
        data = (DataPtr)pend;
    }

    auto end_bpos = end % ElemSize;
    if( end_bpos > 0 ) {
        union {
            ElemType elem;
            char b[ElemSize];
        };
        elem = m_buf[end_epos];
        std::copy( b, b + end_bpos, data );
    }
}

int IRamBufferStream::read( char* data, size_t dataSize ) {
    size_t total_bytes_to_read = std::min( size(), dataSize );
    return WrapBufOps( data, total_bytes_to_read, m_rpos, &IRamBufferStream::readChunk );
}

bool IRamBufferStream::flush() {
    m_wpos = 0;
    m_rpos = 0;
    return true;
}

bool IRamBufferStream::close() {
    bool isFlushed = flush();
    heap_caps_free( m_buf );
    m_buf = nullptr;
    return isFlushed;
};
}  // namespace common
