#pragma once

#include <IHTTPData.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace network {

class HTTPFile : public network::IHTTPDataIn {
public:
    static constexpr size_t BUFFER_SIZE = 8 * 1024u;

    HTTPFile( const std::string& file );
    HTTPFile( const HTTPFile& other ) = delete;
    HTTPFile( HTTPFile&& other ) = delete;
    HTTPFile& operator=( const HTTPFile& other ) = delete;
    HTTPFile& operator=( HTTPFile&& other ) = delete;
    ~HTTPFile();
    void setDataLen( size_t len ) override;
    void setDataType( const char* type ) override;
    void setIsChunked( bool chunked ) override;
    network::HTTPDataResult write( const char* buf, size_t len ) override;
    void writeReset() override;
    uint32_t getWrittenByteCount() const;

private:
    bool pushToBuffer( const char* data, size_t len );
    bool flushBuffer();
    bool writeToFile( const char* data, size_t len );

private:
    std::vector<char> m_buffer;
    uint32_t m_writtenByteCount = 0;
    std::string m_filename;
};

}  // namespace network
