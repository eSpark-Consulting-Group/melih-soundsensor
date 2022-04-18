#include <HTTPWriterOTA.h>
#include <DebugUtils.h>
#include <NmaBroadcastsSender.h>

namespace network {

HTTPWriterOTA::HTTPWriterOTA( esp_ota_handle_t otaHandle, uint32_t operationId )
: m_targetHandle( otaHandle )
, m_operationId( operationId ) {}

void HTTPWriterOTA::setDataLen( size_t len ) {
    if( m_fullDatalen == 0 ) {
        m_fullDatalen = len;
        FUNC_LOGI( "Update started with recvContentLength = %zu", m_fullDatalen );
    }
}

void HTTPWriterOTA::setDataType( const char* type ) {}

void HTTPWriterOTA::setIsChunked( bool chunked ) {}

HTTPDataResult HTTPWriterOTA::write( const char* buf, size_t len ) {
    if( !m_digestCalculator.updateDigest( buf, len ) ) {
        FUNC_LOGE( "Digest update failed" );
        return HTTPDataResult::Error;
    }

    auto retCode = esp_ota_write( m_targetHandle, buf, len );
    if( retCode != ESP_OK ) {
        FUNC_LOGE( "esp_ota_write failed, %d", retCode );
        return HTTPDataResult::Error;
    }
    m_writtenCount += len;
    if( m_writtenCount >= m_fullDatalen / WrittenStep ) {
        m_writtenInPercent += WrittenStep;
        if( m_writtenInPercent > WrittenStep && m_writtenInPercent < 100 ) {
            nma::BroadcastsSender::sendUpdatingFirmware(
                m_writtenInPercent, m_operationId, nma::BroadcastsSender::Process );
        }
        if( m_writtenInPercent > ( 100 - WrittenStep ) ) {
            m_writtenInPercent = 0;
        }
        m_writtenCount = 0;
    }
    return HTTPDataResult::OK;
}

bool HTTPWriterOTA::getResultDigest( char* buffer, size_t bufferSize ) {
    if( bufferSize < 32 ) {
        return false;
    }

    return m_digestCalculator.getDigest( buffer, bufferSize );
}

void HTTPWriterOTA::writeReset() {}
}  // namespace network
