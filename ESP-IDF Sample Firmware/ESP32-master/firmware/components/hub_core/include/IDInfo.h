#pragma once
#include <cstdint>
#include "MemoryMappedFlash.h"
#include "HubIDData.h"
#include "ConnectionIDData.h"
#include <memory>
#include <array>
#include <ProvisionData.h>

namespace core {

class IDInfo {
private:
    struct BlockHeader {
        std::uint16_t sn;
        std::uint16_t ver;
        const char* memory_offset;
        std::size_t flash_offset;
    };

    IDInfo()
    : m_initialized( false ){};

public:
    static IDInfo& get();

    // Hub ID info
    std::uint64_t id() const;
    static constexpr std::size_t UUID_SIZE = 16;
    const std::array<char, UUID_SIZE> uuid() const;
    const char* zwaveRegion() const;
    const char* defaultWiFiSSID() const;
    const char* defaultWiFiPass() const;
    // Connection ID info
    int configRevision() const;
    const char* provisioningServerURL() const;
    std::string provisioningServerToken() const;
    const char* cloudServerURL() const;
    const char* CACertificate() const;
    const char* TLSPrivateKey() const;
    const char* TLSPublicCertificate() const;

    const char* productName() const;

    int writeProvisionData( const ProvisionData& provisionData );

    bool isInitialized() {
        return m_initialized;
    }

    bool init();

private:
    void deinit();
    bool initHubID();
    bool initConnectionID();
    void reset();

public:
    static constexpr auto ID_PARTITION_NAME = "id";
    static constexpr std::size_t ID_PARTITION_SIZE = 0x10000;
    static constexpr auto ID_PARTITION_TYPE = 0x40;
    static constexpr auto ID_PARTITION_SUBTYPE = 0x00;
    // Partitions
    static constexpr std::size_t CONNECTION_ID_0_OFFSET = 0x0;
    static constexpr std::size_t CONNECTION_ID_1_OFFSET = 0x7000;
    static constexpr std::size_t CONNECTION_ID_SIZE = 0x7000;
    static constexpr std::size_t HUB_ID_0_OFFSET = 0xE000;
    static constexpr std::size_t HUB_ID_1_OFFSET = 0xF000;
    static constexpr std::size_t HUB_ID_SIZE = 0x1000;
    // Data SN and Version
    static constexpr std::size_t SN_OFFSET = 0;
    static constexpr std::size_t SN_SIZE = 2;
    static constexpr std::size_t VER_OFFSET = 2;
    static constexpr std::size_t VER_SIZE = 2;
    static constexpr std::uint16_t SN_AND_VER_INVALID_VALUE = 0xFFFF;

private:
    bool m_initialized;

    hard::MemoryMappedFlash m_mmapFlash;
    std::unique_ptr<HubIDData> m_hubID;
    std::unique_ptr<ConnectionIDData> m_connectionID;
    BlockHeader m_activeConnectionIDBlock;
    BlockHeader m_freeConnectionIDBlock;

    BlockHeader m_activeHubIDBlock;
    BlockHeader m_freeHubIDBlock;
};

}  // namespace core
