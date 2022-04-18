#pragma once

#include "DeviceFirmwareUpdateManager.h"
#include <HTTPRequest.h>

namespace common {

class DefaultDownloader : public DeviceFirmwareUpdateManager::Downloader {
public:
    DefaultDownloader( const DeviceFirmwareUpdateManager::ExtendedLink& link,
                       const DeviceFirmwareUpdateManager::Path& destinationPath,
                       ResultCallback resultcallback );

    bool isOk() const override;

    uint8_t getProgress() const override;

private:
    network::HTTPRequest m_request;
    uint8_t m_progress = 0;
    bool m_isOk = true;
    bool checksumCompare( const char* destinationPath, const char* checksumDestinationPath );

private:
    bool isLink( const DeviceFirmwareUpdateManager::Link& link );
    void progressHandler( uint8_t progress );
    std::string getPath( const std::string& filePath );
};

class DefaultDeviceFirmwareUpdateDelegate : public DeviceFirmwareUpdateManager::Delegate {
public:
    DeviceFirmwareUpdateManager::DownloaderConstUPtr createFirmwareDownloader(
        const DeviceFirmwareUpdateManager::ExtendedLink& link,
        const DeviceFirmwareUpdateManager::Path& destinationPath,
        DeviceFirmwareUpdateManager::Downloader::ResultCallback resultCallback ) final;

    DeviceFirmwareUpdateManager::Path getDestinationForLink( const DeviceFirmwareUpdateManager::Link& link ) final;
};

}  // namespace common
