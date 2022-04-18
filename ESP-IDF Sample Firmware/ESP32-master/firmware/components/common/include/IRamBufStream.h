#pragma once

#include <IWriteStream.h>
#include <IReadStream.h>
#include <cstdint>

namespace common {

class IRamBufferStream final : public IWriteStream, public IReadStream {
public:
    constexpr static auto MinBufSize = ( 2048 );
    constexpr static auto MiddleBufSize = ( 16384 );
    constexpr static auto BigBufSize = ( 16384 + 4096 );

    using ElemType = uint32_t;
    using ElemTypePtr = ElemType*;
    using DataPtr = char*;
    static constexpr auto ElemSize = sizeof( ElemType );

    IRamBufferStream( const size_t _buf_len );

    ~IRamBufferStream();

public:
    int write( const char* data, size_t dataSize ) override;

    int read( char* data, size_t dataSize ) override;

    using IWriteStream::write;

    using IReadStream::read;

    bool flush() override;

    bool close() override;

    size_t size() const override {
        return m_rpos > m_wpos ? m_bufsize - m_rpos + m_wpos : m_wpos - m_rpos;
    }

    size_t free() const {
        return ( m_rpos > m_wpos ? m_rpos - m_wpos : m_bufsize - m_wpos + m_rpos ) - 1;
    }

    size_t capacity() const {
        return m_bufsize - 1;
    };

    operator bool() const {
        return nullptr != m_buf;
    }

private:
    template <typename Data, typename Clbk>
    size_t WrapBufOps( Data data, size_t dataSize, size_t& _ctr, Clbk _clbk );

    void writeChunk( size_t pos, const char* data, size_t dataSize );

    void readChunk( size_t pos, char* data, size_t dataSize );

    ElemTypePtr m_buf;
    size_t m_wpos = 0, m_rpos = 0;
    size_t m_bufsize;
};

}  // namespace common
