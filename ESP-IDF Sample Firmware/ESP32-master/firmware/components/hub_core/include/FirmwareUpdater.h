#pragma once

#include <HTTPClient.h>
#include <TCPSocket.h>
#include <HTTPWriterOTA.h>
#include <Mutex.h>
#include <esp_ota_ops.h>
#include <stdint.h>
#include <string>
#include <FirmwareVersion.h>
#include <esp_partition.h>
#include "IWriteStream.h"

namespace core {

class TaskParam;

class FirmwareUpdater {
public:
    static bool update( std::string&& nmaServer, std::string&& version, std::string&& server );
    static bool updateNew( std::string&& imageURL, std::string&& signURL );  // need to new command

    static inline bool wasFirmwareUpdated() {
        return m_firmwareWasUpdated;
    }

    static void checkFimwareUpdatedFlag();
    static bool isFirmwareUpdateInProgress();
    static void resetFirmwareUpdateFlag();

    static bool sendUpdateNotifyIfNeeded();

    void saveFimwareUpdatedFlag();

    static void saveFimwareUpdateStartedFlag( bool val );
    static bool isFimwareUpdateStarted();

private:
    static void task( TaskParam* params );
    static void task1( void* /*pvParameters*/ );

    FirmwareUpdater( std::string&& version, std::string&& server );
    FirmwareUpdater( std::string&& imageURL, std::string&& signURL, const char* signature );

    bool updateOta( const char* version, const char* serverUrl );
    bool updateOtaByTwoLinks( const char* imageURL, const char* signURL );

    bool downloadOta( const char* version, const char* serverUrl );
    bool downloadOtaSignImg( const char* imageURL, const char* signURL );
    bool initOtaPartitionToWrite();

private:
    static constexpr auto UpdatesServerPath = "updates";
    static constexpr auto HubPlatform = HARDWARE_ARCH;
    static constexpr auto HubBoardRevision = HARDWARE_VERSION;
    static constexpr auto FirmwareFileExtension = "bin";
    static constexpr auto FirmwareChecksumFileExtension = "sha256";

    static FirmwareUpdater* m_updater;
    static common::Mutex m_updaterMutex;
    static bool m_firmwareWasUpdated;
    static bool m_wasNotificationSended;

    std::string m_version;
    std::string m_server;
    std::string m_imgUrl;
    std::string m_signUrl;
    network::TCPSocket m_tcpSock;
    network::HTTPClient m_httpClient;
    const esp_partition_t* m_partitionToWrite;
    uint32_t m_operationId;
};

}  // namespace core
