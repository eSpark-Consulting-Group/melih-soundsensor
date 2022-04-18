#include <FirmwareUpdater.h>
#include <DebugUtils.h>
#include <Ezlo.h>
#include <LockKeeper.h>
#include <Utils.h>
#include <HardUtils.h>
#include <NmaBroadcastsSender.h>
#include <BinaryFileStorageFactory.h>
#include <StorageSection.h>
#include <StorageCommonSection.h>
#include "IDInfo.h"

#include "Bus.h"
#include "cloud_client/commands/FirmwareUpdateCompleted.h"

#include <string.h>

#include <mbedtls/ecdsa.h>
#include "mbedtls/ssl.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace core {

FirmwareUpdater* FirmwareUpdater::m_updater = nullptr;
common::Mutex FirmwareUpdater::m_updaterMutex;
bool FirmwareUpdater::m_firmwareWasUpdated = false;
bool FirmwareUpdater::m_wasNotificationSended = false;

struct TaskParam {
    TaskParam( std::string&& _nmaServer )
    : nmaServer( std::move( _nmaServer ) ) {}

    std::string nmaServer;
};

bool FirmwareUpdater::update( std::string&& nmaServer, std::string&& version, std::string&& server ) {
    /// TODO: impelemnt normal try lock (time zero)
    common::LockKeeper<common::Mutex, common::Mutex::Timeout> lock( m_updaterMutex,
                                                                    common::Mutex::Timeout( 1 ) );  // try lock

    if( lock.isLocked() && !m_updater ) {
        m_updater = new FirmwareUpdater( std::move( version ), std::move( server ) );
        if( m_updater ) {
            auto param = std::make_unique<TaskParam>( std::move( nmaServer ) );
            TaskHandle_t handleOfTask;
            BaseType_t taskResult = xTaskCreate( reinterpret_cast<TaskFunction_t>( task ),
                                                 "FWUPDATE_TASK",
                                                 6 * 1024,
                                                 param.release(),
                                                 10,
                                                 &handleOfTask );
            if( pdPASS == taskResult ) {
                return true;  // success
            }

            delete m_updater;
            m_updater = nullptr;
        }
    }

    return false;
}

void FirmwareUpdater::task( TaskParam* params ) {
    /// INFO: block used for corrected res free
    {
        common::LockKeeper<common::Mutex, common::Mutex::Timeout> lock( m_updaterMutex,
                                                                        common::Mutex::Timeout( 0 ) );  // infinity

        if( m_updater ) {
            /// TODO: core::Ezlo::get().getNmaServerPort() ? HTTPS ?
            std::array<char, 64> serverName;
            snprintf( serverName.data(), serverName.size(), "http://%s:%hu", params->nmaServer.c_str(), 80 );

            m_updater->m_server = serverName.data();
            m_updater->updateOta( m_updater->m_version.c_str(), m_updater->m_server.c_str() );

            delete m_updater;
            m_updater = nullptr;
        }
    }

    delete params;

    bus::sendFromNowhere<cloud_client::commands::FirmwareUpdateCompleted>( cloud_client::commands::BUS_NAME );

    vTaskDelete( NULL );
}

void FirmwareUpdater::task1( void* /*pvParameters*/ ) {
    /// INFO: block used for corrected res free
    {
        common::LockKeeper<common::Mutex, common::Mutex::Timeout> lock( m_updaterMutex,
                                                                        common::Mutex::Timeout( 0 ) );  // infinity

        if( m_updater ) {
            /// TODO: core::Ezlo::get().getNmaServerPort() ? HTTPS ?
            m_updater->updateOtaByTwoLinks( m_updater->m_imgUrl.c_str(), m_updater->m_signUrl.c_str() );

            delete m_updater;
            m_updater = nullptr;
        }
    }

    bus::sendFromNowhere<cloud_client::commands::FirmwareUpdateCompleted>( cloud_client::commands::BUS_NAME );

    vTaskDelete( NULL );
}

FirmwareUpdater::FirmwareUpdater( std::string&& version, std::string&& server )
: m_version( std::move( version ) )
, m_server( std::move( server ) )
, m_httpClient( m_tcpSock )
, m_partitionToWrite( nullptr )
, m_operationId( 0 ) {}

FirmwareUpdater::FirmwareUpdater( std::string&& imageURL, std::string&& signURL, const char* signature )
: m_imgUrl( std::move( imageURL ) )
, m_signUrl( std::move( signURL ) )
, m_httpClient( m_tcpSock )
, m_partitionToWrite( nullptr )
, m_operationId( 0 ) {}

bool FirmwareUpdater::isFirmwareUpdateInProgress() {
    uint32_t operationId = 0;
    const auto update_key = storage::section::common::UPD_OPERATION_ID;
    auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::COMMON );
    if( !store ) {
        FUNC_LOGE( "Failed to open storage" );
        return false;
    }
    store->getInt32( update_key, *(int32_t*)( &operationId ) );
    return operationId != 0;
}

void FirmwareUpdater::resetFirmwareUpdateFlag() {
    uint32_t operationId = 0;
    const auto update_key = storage::section::common::UPD_OPERATION_ID;
    auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::COMMON );
    if( !store ) {
        FUNC_LOGE( "Failed to open storage" );
        return;
    }
    store->setInt32( update_key, 0 );
    m_firmwareWasUpdated = false;
}

void FirmwareUpdater::checkFimwareUpdatedFlag() {
    uint32_t operationId = 0;
    const auto update_key = storage::section::common::UPD_OPERATION_ID;
    auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::COMMON );
    if( !store ) {
        FUNC_LOGE( "Failed to open storage" );
        return;
    }
    if( !store->getInt32( update_key, *(int32_t*)( &operationId ) ) ) {
        FUNC_LOGI( "Firmware was not updated" );
    }

    if( operationId != 0 ) {
        FUNC_LOGI( "Updating Firmware check flag >>>> Read from NVS 0x%X", operationId );
        store->setInt32( update_key, 0 );
        FUNC_LOGI( "Updating Firmware flag reset >>>> Saved empty string to NVS" );
        m_firmwareWasUpdated = true;
    }
    else {
        FUNC_LOGI( "Updating Firmware not happend >>>> operation ID = %zu", operationId );
    }
}

bool FirmwareUpdater::sendUpdateNotifyIfNeeded() {
    if( !wasFirmwareUpdated() || m_wasNotificationSended ) {
        return false;
    }

    const auto operationId = hard::common::generateUniqueId();
    nma::BroadcastsSender::sendUpdatingFirmware( 100, operationId, nma::BroadcastsSender::Finished );
    m_wasNotificationSended = true;
    return true;
}

void FirmwareUpdater::saveFimwareUpdatedFlag() {
    if( m_operationId != 0 ) {
        const auto update_key = storage::section::common::UPD_OPERATION_ID;
        auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::COMMON );
        if( !store ) {
            FUNC_LOGE( "Failed to open storage" );
            return;
        }
        store->setInt32( update_key, *(uint32_t*)( &m_operationId ) );
        FUNC_LOGI( " FirmwareUpdater before reboot >>>> Save flag as operationId = 0x%X", m_operationId );
        m_operationId = 0;
    }
}

bool FirmwareUpdater::isFimwareUpdateStarted() {
    auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::COMMON );
    if( !store ) {
        FUNC_LOGE( "Failed to open storage" );
        return false;
    }
    const auto update_key = storage::section::common::UPD_OPERATION_DID_SATRT;
    uint32_t flag = 0;
    store->getInt32( update_key, *(int32_t*)( &flag ) );
    return flag != 0;
}

void FirmwareUpdater::saveFimwareUpdateStartedFlag( bool val ) {
    const auto update_key = storage::section::common::UPD_OPERATION_DID_SATRT;
    auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::COMMON );
    if( !store ) {
        FUNC_LOGE( "Failed to open storage" );
        return;
    }

    store->setInt32( update_key, (int32_t)val );
}

bool FirmwareUpdater::updateOta( const char* version, const char* serverUrl ) {
    m_operationId = hard::common::generateUniqueId();
    nma::BroadcastsSender::sendUpdatingFirmware(
        network::HTTPWriterOTA::WrittenStep, m_operationId, nma::BroadcastsSender::Started );
    FUNC_LOGI( "Update firmware version: %s server: %s", version, serverUrl );
    // Find partition
    if( !initOtaPartitionToWrite() ) {
        FUNC_LOGE( "Cannot get partition to write" );
        nma::BroadcastsSender::sendUpdatingFirmwareError( nma::Errors::UpdateOperationsError, m_operationId );
        return false;
    }

    // download and write OTA to found partition
    if( !downloadOta( version, serverUrl ) ) {
        FUNC_LOGE( "Download firmware failed" );
        m_partitionToWrite = nullptr;
        return false;
    }

    // select updated OTA to load
    esp_err_t espRetCode;
    espRetCode = esp_ota_set_boot_partition( m_partitionToWrite );
    if( espRetCode != ESP_OK ) {
        FUNC_LOGE( "Setting boot partition failed, return code = %d", espRetCode );
        m_partitionToWrite = nullptr;
        nma::BroadcastsSender::sendUpdatingFirmwareError( nma::Errors::UpdateOperationsError, m_operationId );
        return false;
    }
    else {
        saveFimwareUpdatedFlag();
        FUNC_LOGI( "Update success, reboot..." );

        auto ezlo = core::Ezlo::get();
        if( !ezlo ) {
            return false;
        }

        ezlo->reboot();
    }
    return true;
}

bool FirmwareUpdater::downloadOta( const char* version, const char* serverUrl ) {
    constexpr auto urlBufferSize = 100;
    constexpr auto httpRequestTimeout = 10'000;
    char urlBuffer[urlBufferSize];
    esp_ota_handle_t otaHandle = {};

    esp_err_t espRetCode;

    if( m_partitionToWrite == nullptr ) {
        FUNC_LOGE( "Partition isn't initialized" );
        nma::BroadcastsSender::sendUpdatingFirmwareError( nma::Errors::UpdateOperationsError, m_operationId );
        return false;
    }

    espRetCode = esp_ota_begin( m_partitionToWrite, OTA_SIZE_UNKNOWN, &otaHandle );

    if( espRetCode != ESP_OK ) {
        FUNC_LOGE( "esp_ota_begin failed, return code: %d", espRetCode );
        nma::BroadcastsSender::sendUpdatingFirmwareError( nma::Errors::UpdateOperationsError, m_operationId );
        return false;
    }
    network::HTTPWriterOTA otaWriter( otaHandle, m_operationId );

    snprintf( urlBuffer,
              urlBufferSize,
              "%s/%s/%s-%s-%s.%s",
              serverUrl,
              UpdatesServerPath,
              HubPlatform,
              HubBoardRevision,
              version,
              FirmwareFileExtension );

    auto retCode = m_httpClient.get( urlBuffer, &otaWriter, httpRequestTimeout );

    if( retCode != network::HTTPResult::OK ) {
        FUNC_LOGE( "Download firmware failed, result = %d", static_cast<int>( retCode ) );
        nma::BroadcastsSender::sendUpdatingFirmwareError( nma::Errors::CantDownloadData, m_operationId );
        return false;
    }

    constexpr auto digestSize = 32;
    constexpr auto signSize = 255;
    char localDigest[digestSize];
    char sign[signSize + 1];  // It's need by http client
    std::fill( sign, sign + signSize, 0 );

    if( !otaWriter.getResultDigest( localDigest, 32 ) ) {
        FUNC_LOGE( "Firmware cannot verified, get digest failed" );
        nma::BroadcastsSender::sendUpdatingFirmwareError( nma::Errors::CantVerified, m_operationId );
        return false;
    }

    snprintf( urlBuffer,
              urlBufferSize,
              "%s/%s/%s-%s-%s.%s.%s",
              serverUrl,
              UpdatesServerPath,
              HubPlatform,
              HubBoardRevision,
              version,
              FirmwareFileExtension,
              FirmwareChecksumFileExtension );

    retCode = m_httpClient.get( urlBuffer, sign, signSize + 1, httpRequestTimeout );

    if( retCode != network::HTTPResult::OK ) {
        FUNC_LOGE( "Download digest file failed, request url: %s, result = %s",
                   urlBuffer,
                   m_httpClient.GetErrorMessage( retCode ) );
        nma::BroadcastsSender::sendUpdatingFirmwareError( nma::Errors::CantDownloadData, m_operationId );
        return false;
    }

    // check sign
    mbedtls_ecdsa_context ecdsa;
    mbedtls_ecdsa_init( &ecdsa );
    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init( &cert );
    const char* cacert = IDInfo::get().CACertificate();

    if( cacert == nullptr ) {
        FUNC_LOGE( "Cannot get certificate" );
        return false;
    }

    if( mbedtls_x509_crt_parse( &cert, reinterpret_cast<const unsigned char*>( cacert ), std::strlen( cacert ) + 1 ) !=
        0 ) {
        FUNC_LOGE( "Cannot parse certificate" );
        nma::BroadcastsSender::sendUpdatingFirmwareError( nma::Errors::CantVerified, m_operationId );
        return false;
    }

    auto ret = mbedtls_ecdsa_from_keypair( &ecdsa, mbedtls_pk_ec( cert.pk ) );
    if( ret != 0 ) {
        FUNC_LOGE( "mbedtls_ecdsa_from_keypair failed" );
        nma::BroadcastsSender::sendUpdatingFirmwareError( nma::Errors::CantVerified, m_operationId );
        return false;
    }

    ret = mbedtls_ecdsa_read_signature( &ecdsa,
                                        (unsigned char*)localDigest,
                                        32,
                                        reinterpret_cast<unsigned char*>( sign + 1 ),
                                        static_cast<size_t>( sign[0] ) );

    FUNC_LOGI( "mbedtls_ecdsa_read_signature returned %d", ret );
    if( ret != 0 && ret != MBEDTLS_ERR_ECP_SIG_LEN_MISMATCH ) {
        FUNC_LOGE( "signature incorrect" );
        nma::BroadcastsSender::sendUpdatingFirmwareError( nma::Errors::CantVerified, m_operationId );
        return false;
    }

    espRetCode = esp_ota_end( otaHandle );
    if( espRetCode != ESP_OK ) {
        FUNC_LOGE( "esp_ota_end failed, return code: %d", espRetCode );
        nma::BroadcastsSender::sendUpdatingFirmwareError( nma::Errors::UpdateOperationsError, m_operationId );
        return false;
    }

    return true;
}

bool FirmwareUpdater::initOtaPartitionToWrite() {
    m_partitionToWrite = esp_ota_get_next_update_partition( nullptr );
    if( m_partitionToWrite == nullptr ) {
        FUNC_LOGE( "Cannot get partition to write" );
        return false;
    }

    return true;
}
bool FirmwareUpdater::updateOtaByTwoLinks( const char* imageURL, const char* signURL ) {
    m_operationId = hard::common::generateUniqueId();
    nma::BroadcastsSender::sendUpdatingFirmware(
        network::HTTPWriterOTA::WrittenStep, m_operationId, nma::BroadcastsSender::Started );
    FUNC_LOGI( "Update firmware:  %s sign: %s", imageURL, signURL );
    // Find partition
    if( !initOtaPartitionToWrite() ) {
        FUNC_LOGE( "Cannot get partition to write" );
        nma::BroadcastsSender::sendUpdatingFirmwareError( nma::Errors::UpdateOperationsError, m_operationId );
        return false;
    }

    saveFimwareUpdateStartedFlag( true );

    // download and write OTA to found partition
    if( !downloadOtaSignImg( imageURL, signURL ) ) {
        FUNC_LOGE( "Download firmware failed" );
        m_partitionToWrite = nullptr;
        return false;
    }

    // select updated OTA to load
    esp_err_t espRetCode;
    espRetCode = esp_ota_set_boot_partition( m_partitionToWrite );
    if( espRetCode != ESP_OK ) {
        FUNC_LOGE( "Setting boot partition failed, return code = %d", espRetCode );
        m_partitionToWrite = nullptr;
        nma::BroadcastsSender::sendUpdatingFirmwareError( nma::Errors::UpdateOperationsError, m_operationId );
        return false;
    }

    saveFimwareUpdatedFlag();
    auto ezlo = core::Ezlo::get();
    if( !ezlo ) {
        return false;
    }

    ezlo->forceReinstallDefaultPlugins();
    FUNC_LOGI( "Update success, reboot..." );
    ezlo->reboot();
    return true;
}

bool FirmwareUpdater::downloadOtaSignImg( const char* imageURL, const char* signURL ) {
    constexpr auto urlBufferSize = 100;
    constexpr auto httpRequestTimeout = 10'000;
    char urlBuffer[urlBufferSize];
    esp_ota_handle_t otaHandle = {};

    esp_err_t espRetCode;

    if( m_partitionToWrite == nullptr ) {
        FUNC_LOGE( "Partition isn't initialized" );
        nma::BroadcastsSender::sendUpdatingFirmwareError( nma::Errors::UpdateOperationsError, m_operationId );
        return false;
    }

    espRetCode = esp_ota_begin( m_partitionToWrite, OTA_SIZE_UNKNOWN, &otaHandle );

    if( espRetCode != ESP_OK ) {
        FUNC_LOGE( "esp_ota_begin failed, return code: %d", espRetCode );
        nma::BroadcastsSender::sendUpdatingFirmwareError( nma::Errors::UpdateOperationsError, m_operationId );
        return false;
    }
    network::HTTPWriterOTA otaWriter( otaHandle, m_operationId );

    auto retCode = m_httpClient.get( imageURL, &otaWriter, httpRequestTimeout );

    if( retCode != network::HTTPResult::OK ) {
        FUNC_LOGE( "Download firmware failed, result = %d", static_cast<int>( retCode ) );
        nma::BroadcastsSender::sendUpdatingFirmwareError( nma::Errors::CantDownloadData, m_operationId );
        return false;
    }

    constexpr auto digestSize = 32;
    constexpr auto signSize = 255;
    char localDigest[digestSize];
    char sign[signSize + 1];  // It's need by http client
    std::fill( sign, sign + signSize, 0 );

    if( !otaWriter.getResultDigest( localDigest, 32 ) ) {
        FUNC_LOGE( "Firmware cannot verified, get digest failed" );
        nma::BroadcastsSender::sendUpdatingFirmwareError( nma::Errors::CantVerified, m_operationId );
        return false;
    }

    retCode = m_httpClient.get( signURL, sign, signSize + 1, httpRequestTimeout );

    if( retCode != network::HTTPResult::OK ) {
        FUNC_LOGE( "Download digest file failed, request url: %s, result = %s",
                   urlBuffer,
                   m_httpClient.GetErrorMessage( retCode ) );
        nma::BroadcastsSender::sendUpdatingFirmwareError( nma::Errors::CantDownloadData, m_operationId );
        return false;
    }

    // check sign
    mbedtls_ecdsa_context ecdsa;
    mbedtls_ecdsa_init( &ecdsa );
    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init( &cert );
    const char* cacert = IDInfo::get().CACertificate();

    if( cacert == nullptr ) {
        FUNC_LOGE( "Cannot get certificate" );
        return false;
    }

    if( mbedtls_x509_crt_parse( &cert, reinterpret_cast<const unsigned char*>( cacert ), std::strlen( cacert ) + 1 ) !=
        0 ) {
        FUNC_LOGE( "Cannot parse certificate" );
        nma::BroadcastsSender::sendUpdatingFirmwareError( nma::Errors::CantVerified, m_operationId );
        return false;
    }

    auto ret = mbedtls_ecdsa_from_keypair( &ecdsa, mbedtls_pk_ec( cert.pk ) );
    if( ret != 0 ) {
        FUNC_LOGE( "mbedtls_ecdsa_from_keypair failed" );
        nma::BroadcastsSender::sendUpdatingFirmwareError( nma::Errors::CantVerified, m_operationId );
        return false;
    }

    ret = mbedtls_ecdsa_read_signature( &ecdsa,
                                        (unsigned char*)localDigest,
                                        32,
                                        reinterpret_cast<unsigned char*>( sign + 1 ),
                                        static_cast<size_t>( sign[0] ) );

    FUNC_LOGI( "mbedtls_ecdsa_read_signature returned %d", ret );
    if( ret != 0 && ret != MBEDTLS_ERR_ECP_SIG_LEN_MISMATCH ) {
        FUNC_LOGE( "signature incorrect" );
        nma::BroadcastsSender::sendUpdatingFirmwareError( nma::Errors::CantVerified, m_operationId );
        return false;
    }

    espRetCode = esp_ota_end( otaHandle );
    if( espRetCode != ESP_OK ) {
        FUNC_LOGE( "esp_ota_end failed, return code: %d", espRetCode );
        nma::BroadcastsSender::sendUpdatingFirmwareError( nma::Errors::UpdateOperationsError, m_operationId );
        return false;
    }

    return true;
}

bool FirmwareUpdater::updateNew( std::string&& imageURL, std::string&& signURL ) {
    common::LockKeeper<common::Mutex, common::Mutex::Timeout> lock( m_updaterMutex,
                                                                    common::Mutex::Timeout( 1 ) );  // try lock
    if( !lock.isLocked() ) {
        FUNC_LOGE( "Can't acquire mutex!" );
        return false;
    }
    if( m_updater ) {
        FUNC_LOGE( "Updater instance already exist!" );
        return false;
    }

    m_updater = new FirmwareUpdater( std::move( imageURL ), std::move( signURL ), nullptr );
    if( !m_updater ) {
        FUNC_LOGE( "Can't create updater instance!" );
        return false;
    }

    TaskHandle_t handleOfTask;
    BaseType_t taskResult =
        xTaskCreate( &FirmwareUpdater::task1, "FWUPDATE_TASK", 6 * 1024, nullptr, 10, &handleOfTask );
    if( pdPASS != taskResult ) {
        FUNC_LOGE( "Can't create updater task! xTaskCreate return %d", taskResult );
        delete m_updater;
        m_updater = nullptr;
        return false;  // success
    }

    return true;
}

}  // namespace core
