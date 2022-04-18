#pragma once

#include <string>
#include <cinttypes>

#include <GatewayDefs.h>
#include <GatewayID.h>
#include <PagesManager.h>
#include <list>

namespace storage {
class File;
}  // namespace storage

namespace common {
class Buffer;
}  // end of namespace common

namespace gateways {

class DeviceSettingsManager final {
public:
    enum class Result {
        OK = 0,

        WrongArguments,
        LackOfResources,
        FileNotExist,
        SystemError
    };

public:
    static DeviceSettingsManager& get();

    Result setDeviceName( const DeviceId& deviceId, GatewayID gatewayId, const char* name );

    Result moveDeviceToPage( const DeviceId& deviceId, GatewayID gatewayId, const core::PagesManager::PageId& pageId );

    Result readDeviceSettings( const DeviceId& deviceId,
                               GatewayID gatewayId,
                               char* name,
                               size_t maxNameSize,
                               core::PagesManager::PageId& pageId );

    Result readDeviceName( const DeviceId& deviceId, GatewayID gatewayId, std::string& nameBuffer );

    Result readPageId( const DeviceId& deviceId, GatewayID gatewayId, core::PagesManager::PageId& pageId );

    Result onItemRemoved( const uint32_t& itemId, bool pageObject = true );

    Result onAllPagesRemoved();

    Result onDeviceRemoved( const DeviceId& deviceId, GatewayID gatewayId );

    std::list<DeviceId> getDevicesByPage( const core::PagesManager::PageId& pageId );

public:
    constexpr static size_t MaxNameSize = 64;
    constexpr static uint16_t VendorIdMaxSize = 32;

private:
    constexpr static char DeviceSettingsFileTemplate[] = "%s/%" PRIX32 ".%c%c";

    constexpr static uint8_t StorageMajorVersion = 2;
    constexpr static uint8_t StorageMinorVersion = 0;

private:
    Result openStorageForAdding( storage::File& file, const DeviceId& deviceId, GatewayID gatewayId );

    Result openStorageForReading( storage::File& file, const DeviceId& deviceId, GatewayID gatewayId );

    bool fillDeviceSettingPath( char* buffer,
                                size_t maxBufferSize,
                                const DeviceId& deviceId,
                                GatewayID gatewayId ) const;

    bool readAndCheckVersion( storage::File& file );

    bool updateVersionHeader( storage::File& file );

    bool writeVersionHeader( storage::File& file );
};

}  // namespace gateways
