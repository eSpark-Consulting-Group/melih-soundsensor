#include "UWebSocket.h"

#include "Ezlo.h"
#include "UWebSocket.h"

#include <sstream>
#include <iomanip>

#include "DebugUtils.h"

#include <sys/socket.h>

#include <string.h>
#include <random>
#include <cstdio>

namespace network {

namespace {

static constexpr uint16_t CLOSE_CANNOT_ACCEPT = 1003;
static constexpr uint16_t CLOSE_NO_STATUS_CODE = 1005;

static constexpr uint16_t CLOSE_NOT_CONSISTENT = 1007;

static constexpr uint16_t CLOSE_NO_EXTENSION = 1010;

static constexpr uint16_t CLOSE_SERVICE_RESTART = 1012;
static constexpr uint16_t CLOSE_TRY_AGAIN_LATER = 1013;

static constexpr auto DefaultSendTimeout = 100;

#define WEB_SOCK_LOG

#ifdef WEB_SOCK_LOG

#define WS_TAG "UWEB_SOCK"
#define WS_LOGE( ... ) ESP_LOGE( WS_TAG, __VA_ARGS__ )
#define WS_LOGW( ... ) ESP_LOGW( WS_TAG, __VA_ARGS__ )
#define WS_LOGI( ... ) ESP_LOGI( WS_TAG, __VA_ARGS__ )
#define WS_LOGD( ... ) ESP_LOGD( WS_TAG, __VA_ARGS__ )

#else

#define WS_LOGE( ... ) \
    do {               \
    } while( false )
#define WS_LOGW( ... ) \
    do {               \
    } while( false )
#define WS_LOGI( ... ) \
    do {               \
    } while( false )
#define WS_LOGD( ... ) \
    do {               \
    } while( false )

#endif

}  // namespace

UWebSocket::UWebSocket( ISocket::UPtr&& socket )
: ProxySocket( std::move( socket ) ) {}

UWebSocket::~UWebSocket() {
    if( isConnected() ) {
        close();
    }
}

bool UWebSocket::connectToHostViaProxySocket( const char* remoteAddress, const uint16_t remotePort ) {
    WS_LOGI( "connectToHostViaProxySocket(address=%s, port=%d)", remoteAddress, remotePort );

    m_lastIoTimepoint = ClockType::now();
    return true;
}

std::string UWebSocket::createHeader( const std::string& url, const std::string& serial ) {
    return "";
}

bool UWebSocket::writeOutBufferToSocket( bool isFinal ) {
    WS_LOGD( ">>>> send(payload size=%d, isFinal=%d):\n%s", m_outBufferBytes, (int)isFinal, m_outBuffer );

    auto length = m_outBufferBytes;

    if( !socket().sendAll( m_outBuffer, length ) ) {
        WS_LOGE( "Can't send data" );
        return false;
    }

    if( isFinal ) {
        socket().sendAll( "\0", 1 );
        m_sendedToSocket = 0;
    }
    else {
        m_sendedToSocket += length;
    }
    m_outBufferBytes = 0;
    m_lastIoTimepoint = ClockType::now();

    return true;
}

bool start = false;

int UWebSocket::send( const char* srcBuffer, const std::size_t srcBufferSize, const int flags ) {
    const bool isFinal = flags & FinalMessage;
    std::size_t bytesProcessed = 0;
    while( true ) {
        auto freeSpaceInBuff = OutputBufferMaxSize - 1 - m_outBufferBytes;
        auto bytesToCopy = std::min<std::size_t>( srcBufferSize - bytesProcessed, freeSpaceInBuff );
        std::memcpy( m_outBuffer + m_outBufferBytes, srcBuffer + bytesProcessed, bytesToCopy );
        bytesProcessed += bytesToCopy;
        m_outBufferBytes += bytesToCopy;
        m_outBuffer[m_outBufferBytes] = '\0';

        if( ( !isFinal || 0 == m_outBufferBytes ) && ( OutputBufferMaxSize - 1 > m_outBufferBytes ) ) {
            return srcBufferSize;
        }

        if( !writeOutBufferToSocket( isFinal ) ) {
            ProxySocket::close();
            reset();
            return -1;
        }
    }  // end of infinite loop
}

int UWebSocket::receive( char* dstBuffer, const std::size_t dstBifferSize ) {
    if( 0 == dstBifferSize || dstBuffer == nullptr ) {
        return 0;
    }

    if( start )
        return 0;

    std::size_t receivedBytes = 0;
    while( true ) {
        if( 0 < m_inBufferBytes ) {
            auto bytesToCopy = std::min( m_inBufferBytes, dstBifferSize );
            std::memcpy( dstBuffer, m_inBuffer + m_inBufferPos, bytesToCopy );
            m_inBufferPos += bytesToCopy;
            receivedBytes += bytesToCopy;
            m_inBufferBytes -= bytesToCopy;

            if( receivedBytes == dstBifferSize ) {
                return dstBifferSize;
            }
        }

        m_inBufferPos = 0;

        auto ret = socket().receive( m_inBuffer, InputBufferMaxSize - 1 );
        if( 0 > ret ) {
            break;
        }
        else if( 0 == ret ) {
            return receivedBytes;
        }

        if( receivedBytes == 0 ) {
            if( ret == 4 )
                if( m_inBuffer[0] == 0x03 && (uint8_t)m_inBuffer[1] == 0xF3 && (uint8_t)m_inBuffer[2] == 0x0F &&
                    (uint8_t)m_inBuffer[3] == 0xC0 ) {
                    ///////////////////// START FW_UPDATE
                    WS_LOGI( "START FW_UPDATE" );
                    uint8_t req[4] = { 0xC0, 0x0F, 0xF3, 0x03 };
                    socket().send( (char*)req, 4 );
                    FW_update();
                    auto ezlo = core::Ezlo::get();
                    ezlo->reboot();
                    return 0;
                }
        }

        m_inBufferBytes = ret;
        m_inBuffer[m_inBufferBytes] = '\0';

        WS_LOGD( "<<<< receive(payload size=%d):\n%s", m_inBufferBytes, m_inBuffer );
    }

    WS_LOGE( "Connection is forced to close" );
    return -1;
}

#include "esp_ota_ops.h"
#include "esp_spi_flash.h"

bool UWebSocket::FW_update() {
    char* inBuf_upd = (char*)malloc( 4096 );
    uint32_t len_upd = 0, real_cou = 0, old_len = 0;
    uint8_t cou_len = 0, tm_cou = 0;

    bool update = true;
    uint32_t adr_fw;
    const esp_partition_t* m_partitionToWrite;

    m_partitionToWrite = esp_ota_get_next_update_partition( nullptr );
    if( m_partitionToWrite == nullptr ) {
        WS_LOGE( "Cannot get partition to write" );
        free( inBuf_upd );
        return false;
    }

    WS_LOGI( "partition to write addr-%X, size-%d ", m_partitionToWrite->address, m_partitionToWrite->size );
    adr_fw = m_partitionToWrite->address;
    while( update ) {
        if( !start ) {
            auto ret = socket().receive( inBuf_upd, 4 );
            if( ret == 0 ) {
                continue;
            }

            for( int i = 0; i < 4; i++ ) {
                len_upd = len_upd + ( inBuf_upd[i] << ( cou_len * 8 ) );
                WS_LOGI( "data bt[%d] =  %d. ", i, inBuf_upd[i] );
                cou_len++;
            }
            if( cou_len == 4 ) {
                start = true;
                old_len = len_upd;
                WS_LOGI( "Len FW %d. ", len_upd );
#ifdef CONFIG_UART_API
                auto ezlo = core::Ezlo::get();
                ezlo->m_isFailedBootCtrReseted = true;
#endif
                tm_cou = 0;
                //                continue;
            }
        }
        auto ret = socket().receive( inBuf_upd + real_cou, 4096 - real_cou );
        if( ret ) {
            tm_cou = 0;
            m_lastIoTimepoint = ClockType::now();
            real_cou = real_cou + ret;
            old_len = old_len - ret;
            WS_LOGI( "Read FW %d. Old len - %d ", ret, old_len );
            if( ( real_cou == 4096 ) || ( old_len == 0 ) ) {
                spi_flash_erase_sector( adr_fw / SPI_FLASH_SEC_SIZE );
                spi_flash_write( adr_fw, inBuf_upd, real_cou );
                adr_fw = adr_fw + SPI_FLASH_SEC_SIZE;

                socket().send( (char*)&real_cou, 4 );
                real_cou = 0;
                WS_LOGI( "SAVE FW to flash. " );
                /// save data to flash
            }
        }
        else {
            tm_cou++;
            if( tm_cou > 150 ) {
                real_cou = 1 << 24;
                socket().send( (char*)&real_cou, 4 );
                WS_LOGE( "FW timeout error" );
                update = false;
                vTaskDelay( 200 / portTICK_PERIOD_MS );
                free( inBuf_upd );
                return false;
            }
        }
        if( old_len == 0 ) {
            update = false;
            WS_LOGI( "FW read complete" );
            sprintf( inBuf_upd, "FW UPDATE complete \r\n" );
            vTaskDelay( 50 / portTICK_PERIOD_MS );
            socket().send( inBuf_upd, strlen( inBuf_upd + 1 ) );
            vTaskDelay( 200 / portTICK_PERIOD_MS );
            break;
        }
        vTaskDelay( 20 / portTICK_PERIOD_MS );
    }
    free( inBuf_upd );

    esp_err_t espRetCode;
    espRetCode = esp_ota_set_boot_partition( m_partitionToWrite );
    if( espRetCode != ESP_OK ) {
        WS_LOGE( "Setting boot partition failed, return code = %d", espRetCode );

        return false;
    }
    auto ezlo = core::Ezlo::get();
    ezlo->forceReinstallDefaultPlugins();
    return true;
}

bool UWebSocket::close() {
    WS_LOGI( "Close web socket" );
    return true;
}

void UWebSocket::reset() {
    m_outBufferBytes = 0;
    m_sendedToSocket = 0;
    m_remainingBytes = 0;
    m_isMask = false;
}

}  // namespace network
