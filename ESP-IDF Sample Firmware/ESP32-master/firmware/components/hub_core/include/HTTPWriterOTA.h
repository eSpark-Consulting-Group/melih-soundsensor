#pragma once

#include <IHTTPData.h>
#include <StreamDigestCalculator.h>
#include <esp_ota_ops.h>

namespace network {

class HTTPWriterOTA : public IHTTPDataIn {
public:
    static constexpr size_t WrittenStep = 10;

    HTTPWriterOTA( esp_ota_handle_t otaHandle, uint32_t operationId );
    void setDataLen( size_t len ) override;
    void setDataType( const char* type ) override;
    void setIsChunked( bool chunked ) override;
    HTTPDataResult write( const char* buf, size_t len ) override;
    void writeReset() override;
    bool getResultDigest( char* buffer, size_t bufferSize );

private:
    core::StreamDigestCalculator m_digestCalculator;
    esp_ota_handle_t m_targetHandle = {};
    size_t m_fullDatalen = 0;
    uint32_t m_writtenCount = 0;
    uint32_t m_operationId = 0;
    uint8_t m_writtenInPercent = 0;
};

}  // namespace network
