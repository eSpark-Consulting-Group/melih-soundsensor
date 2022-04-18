
#pragma once

#include <vector>

#include <IWriteStream.h>

namespace nma {

class ICommand;

class BufferStream final : public common::IWriteStream {
    constexpr static size_t MinBufferLength = 256;

public:
    BufferStream( size_t BufferLength = MinBufferLength );

public:
    int write( const char* buffer, size_t size ) override;

    bool flush() override {
        return true;
    }

    bool close() override;

    const char* data() const {
        return m_sendingBuffer.empty() ? "" : m_sendingBuffer.data();
    }

    size_t size() const {
        return m_writtenBytes;
    }

    void reset() {
        m_sendingBuffer.resize( MinBufferLength );
        m_writtenBytes = 0;
    }

private:
    std::vector<char> m_sendingBuffer;

    size_t m_writtenBytes = 0;
};

}  // namespace nma
