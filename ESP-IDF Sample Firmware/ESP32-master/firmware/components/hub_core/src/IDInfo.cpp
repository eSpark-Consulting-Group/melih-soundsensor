#include "IDInfo.h"
#include <esp_spi_flash.h>
#include <esp_partition.h>
#include "DebugUtils.h"
#include "HubIDDataVer1.h"
#include "HubIDDataVer2.h"
#include "ConnectionIDDataVer1.h"
#include "ConnectionIDDataVer2.h"
#include <FlashUtils.h>
#include <list>
#include <algorithm>
#include <cstring>

namespace core {

IDInfo& IDInfo::get() {
    static IDInfo singleton;
    return singleton;
}

bool IDInfo::init() {
    reset();
    if( !m_mmapFlash.map_partition( ID_PARTITION_NAME ) ) {
        FUNC_LOGE( "Can't map to memory Hub ID partition" );
        return false;
    }
    if( ID_PARTITION_SIZE != m_mmapFlash.size() ) {
        FUNC_LOGE( "ID partition has wrong size" );
        reset();
        return false;
    }
    if( !initHubID() ) {
        FUNC_LOGE( "Can't find Hub ID data" );
        reset();
        return false;
    }
    if( !initConnectionID() ) {
        FUNC_LOGE( "Can't find Connection ID data" );
        reset();
        return false;
    }

    m_initialized = true;

    if( m_hubID->name() ) {
        FUNC_LOGI( "Product name is %s", m_hubID->name() );
    }
    else {
        FUNC_LOGI( "Product name is EMPTY" );
    }

    return true;
}

void IDInfo::deinit() {
    reset();
}

// Hub ID info
std::uint64_t IDInfo::id() const {
    return m_hubID->id();
}

const std::array<char, IDInfo::UUID_SIZE> IDInfo::uuid() const {
    std::array<char, UUID_SIZE> uuid;
    std::copy_n( m_hubID->uuid(), UUID_SIZE, uuid.begin() );
    return uuid;
}

const char* IDInfo::zwaveRegion() const {
    return m_hubID->zwaveRegion();
}

const char* IDInfo::defaultWiFiSSID() const {
    return m_hubID->defaultWiFiSSID();
}

const char* IDInfo::defaultWiFiPass() const {
    return m_hubID->defaultWiFiPass();
}

const char* IDInfo::productName() const {
    return m_hubID->name();
}

// Connection ID info

int IDInfo::configRevision() const {
    return m_connectionID->configRevision();
}

const char* IDInfo::provisioningServerURL() const {
    return m_connectionID->provisioningServerURL();
}

std::string IDInfo::provisioningServerToken() const {
    return std::string( m_connectionID->provisioningServerToken(),
                        ConnectionIDDataVer1::PROVISIONING_SERVER_TOKEN_SIZE );
}

const char* IDInfo::cloudServerURL() const {
    return m_connectionID->cloudServerURL();
}

const char* IDInfo::CACertificate() const {
    return m_connectionID->CACertificate();
}

const char* IDInfo::TLSPrivateKey() const {
    return m_connectionID->TLSPrivateKey();
}

const char* IDInfo::TLSPublicCertificate() const {
    return m_connectionID->TLSPublicCertificate();
}

int IDInfo::writeProvisionData( const ProvisionData& provisionData ) {
    if( !m_connectionID ) {
        FUNC_LOGE( "Connection ID is not initialized" );
        return -1;
    }
    if( !hard::eraseFlash( m_freeConnectionIDBlock.flash_offset, CONNECTION_ID_SIZE ) ) {
        FUNC_LOGE( "Can't erase Connection ID block for new connection data" );
        return -1;
    }
    m_freeConnectionIDBlock.sn = m_activeConnectionIDBlock.sn + 1;
    if( m_freeConnectionIDBlock.sn == SN_AND_VER_INVALID_VALUE ) {
        m_freeConnectionIDBlock.sn = 0;
    }
    m_freeConnectionIDBlock.ver = 2;

    ConnectionIDDataVer2 connection_id( m_freeConnectionIDBlock.memory_offset, m_freeConnectionIDBlock.flash_offset );
    bool result = connection_id.setProvisioningServerURL( provisionData.provisionServer );
    result = result && connection_id.setProvisioningServerToken( provisionData.provisionToken );
    result = result && connection_id.setCloudServerURL( provisionData.cloudServer );
    result = result && connection_id.setTLSPrivateKey( provisionData.sslPrivateKey );
    result = result && connection_id.setTLSPublicCertificate( provisionData.sslSharedKey );
    result = result && connection_id.setCACertificate( provisionData.signingCaCertificate );
    result = result && connection_id.setConfigRevision( provisionData.configRevision );
    result = result && hard::writeToFlash(
                           m_freeConnectionIDBlock.flash_offset + SN_OFFSET, &m_freeConnectionIDBlock.sn, SN_SIZE );
    result = result && hard::writeToFlash(
                           m_freeConnectionIDBlock.flash_offset + VER_OFFSET, &m_freeConnectionIDBlock.ver, VER_SIZE );
    if( result ) {
        if( m_freeConnectionIDBlock.sn == 0 ) {
            FUNC_LOGW( "Need to erase active Connection ID block" );
            if( !hard::eraseFlash( m_activeConnectionIDBlock.flash_offset, CONNECTION_ID_SIZE ) ) {
                FUNC_LOGE( "Can't erase active Connection ID block" );
            }
        }
    }
    else {
        FUNC_LOGE( "Failed writing new Connection ID data" );
        if( !hard::eraseFlash( m_freeConnectionIDBlock.flash_offset, CONNECTION_ID_SIZE ) ) {
            FUNC_LOGE( "Can't cleanup new Connection ID block" );
        }
    }

    if( !m_hubID ) {
        FUNC_LOGE( "Hub ID is not initialized" );
        return -1;
    }
    if( !hard::eraseFlash( m_freeHubIDBlock.flash_offset, HUB_ID_SIZE ) ) {
        FUNC_LOGE( "Can't erase Connection ID block for new connection data" );
        return -1;
    }

    m_freeHubIDBlock.sn = m_activeHubIDBlock.sn + 1;
    if( m_freeHubIDBlock.sn == SN_AND_VER_INVALID_VALUE ) {
        m_freeHubIDBlock.sn = 0;
    }
    m_freeHubIDBlock.ver = 2;

    // m_hubID
    HubIDDataVer2 hub_id( m_freeHubIDBlock.memory_offset, m_freeHubIDBlock.flash_offset );
    result = result && hub_id.setId( m_hubID->id() );
    result = result && hub_id.setUUID( m_hubID->uuid(), UUID_SIZE );
    result = result && hub_id.setZwaveRegion( m_hubID->zwaveRegion() );
    result = result && hub_id.setDefaultWiFiSSID( m_hubID->defaultWiFiSSID() );
    result = result && hub_id.setDefaultWiFiPass( m_hubID->defaultWiFiPass() );
    result = result && hub_id.setProductName( provisionData.productName );

    result = result && hard::writeToFlash( m_freeHubIDBlock.flash_offset + SN_OFFSET, &m_freeHubIDBlock.sn, SN_SIZE );
    result =
        result && hard::writeToFlash( m_freeHubIDBlock.flash_offset + VER_OFFSET, &m_freeHubIDBlock.ver, VER_OFFSET );

    if( result ) {
        if( m_freeHubIDBlock.sn == 0 ) {
            FUNC_LOGW( "Need to erase active Hub ID block" );
            if( !hard::eraseFlash( m_activeHubIDBlock.flash_offset, HUB_ID_SIZE ) ) {
                FUNC_LOGE( "Can't erase active Hub ID block" );
            }
        }
    }
    else {
        FUNC_LOGE( "Failed writing new Hub ID data" );
        if( !hard::eraseFlash( m_freeHubIDBlock.flash_offset, HUB_ID_SIZE ) ) {
            FUNC_LOGE( "Can't cleanup new Hub ID block" );
        }
    }

    m_initialized = false;
    return connection_id.configRevision();
}

bool IDInfo::initHubID() {
    const char* id_partition_pos = reinterpret_cast<const char*>( m_mmapFlash.get() );
    const auto id_pos = m_mmapFlash.get_flash_offset();
    std::list<BlockHeader> id_blocks = { { 0, 0, id_partition_pos + HUB_ID_0_OFFSET, id_pos + HUB_ID_0_OFFSET },
                                         { 0, 0, id_partition_pos + HUB_ID_1_OFFSET, id_pos + HUB_ID_1_OFFSET } };

    assert( sizeof( BlockHeader::sn ) == SN_SIZE );
    assert( sizeof( BlockHeader::ver ) == VER_SIZE );
    for( auto& block : id_blocks ) {
        std::memcpy( &block.sn, block.memory_offset + SN_OFFSET, SN_SIZE );
        std::memcpy( &block.ver, block.memory_offset + VER_OFFSET, VER_SIZE );
    }
    auto valid_end = std::partition( id_blocks.begin(), id_blocks.end(), []( const BlockHeader block ) {
        return block.sn != SN_AND_VER_INVALID_VALUE && block.ver != SN_AND_VER_INVALID_VALUE;
    } );
    if( valid_end == id_blocks.begin() ) {
        FUNC_LOGE( "Can't find valid hub ID block" );
        return false;
    }
    auto active_block =
        std::max_element( id_blocks.begin(), valid_end, []( const BlockHeader& left, const BlockHeader& right ) {
            return left.sn < right.sn;
        } );
    auto free_block = id_blocks.begin();
    while( free_block == active_block ) {
        ++free_block;
    }
    if( free_block == id_blocks.end() ) {
        FUNC_LOGE( "Can't find free hub ID block" );
        return false;
    }
    FUNC_LOGI( "Found valid hub ID block. SN: %u, VER: %u", active_block->sn, active_block->ver );

    if( active_block->ver < 2 ) {
        m_hubID = std::make_unique<HubIDDataVer1>( active_block->memory_offset );
    }
    else if( active_block->ver == 2 ) {
        m_hubID = std::make_unique<HubIDDataVer2>( active_block->memory_offset, active_block->flash_offset );
    }
    else {
        FUNC_LOGE( "Unknown version of connection ID data" );
    }

    if( m_hubID ) {
        m_activeHubIDBlock = *active_block;
        m_freeHubIDBlock = *free_block;
    }

    return (bool)m_hubID;
}

bool IDInfo::initConnectionID() {
    const char* id_partition_pos = reinterpret_cast<const char*>( m_mmapFlash.get() );
    const auto id_pos = m_mmapFlash.get_flash_offset();
    std::list<BlockHeader> id_blocks = {
        { 0, 0, id_partition_pos + CONNECTION_ID_0_OFFSET, id_pos + CONNECTION_ID_0_OFFSET },
        { 0, 0, id_partition_pos + CONNECTION_ID_1_OFFSET, id_pos + CONNECTION_ID_1_OFFSET }
    };
    assert( sizeof( BlockHeader::sn ) == SN_SIZE );
    assert( sizeof( BlockHeader::ver ) == VER_SIZE );
    for( auto& block : id_blocks ) {
        std::memcpy( &block.sn, block.memory_offset + SN_OFFSET, SN_SIZE );
        std::memcpy( &block.ver, block.memory_offset + VER_OFFSET, VER_SIZE );
    }
    auto valid_end = std::partition( id_blocks.begin(), id_blocks.end(), []( const BlockHeader block ) {
        return block.sn != SN_AND_VER_INVALID_VALUE && block.ver != SN_AND_VER_INVALID_VALUE;
    } );
    if( valid_end == id_blocks.begin() ) {
        FUNC_LOGE( "Can't find valid connection ID block" );
        return false;
    }
    auto active_block =
        std::max_element( id_blocks.begin(), valid_end, []( const BlockHeader& left, const BlockHeader& right ) {
            return left.sn < right.sn;
        } );
    auto free_block = id_blocks.begin();
    while( free_block == active_block ) {
        ++free_block;
    }
    if( free_block == id_blocks.end() ) {
        FUNC_LOGE( "Can't find free connection ID block" );
        return false;
    }
    FUNC_LOGI( "Found valid Connection ID block. SN: %u, VER: %u", active_block->sn, active_block->ver );
    if( active_block->ver == 1 ) {
        m_connectionID =
            std::make_unique<ConnectionIDDataVer1>( active_block->memory_offset, active_block->flash_offset );
    }
    else if( active_block->ver == 2 ) {
        m_connectionID =
            std::make_unique<ConnectionIDDataVer2>( active_block->memory_offset, active_block->flash_offset );
    }
    else {
        FUNC_LOGE( "Unknown version of connection ID data" );
    }
    if( m_connectionID ) {
        m_activeConnectionIDBlock = *active_block;
        m_freeConnectionIDBlock = *free_block;
    }
    return (bool)m_connectionID;
}

void IDInfo::reset() {
    m_initialized = false;
    m_hubID.reset();
    m_connectionID.reset();
    m_mmapFlash.unmap();
}

}  // namespace core
