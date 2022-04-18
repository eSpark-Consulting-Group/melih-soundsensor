#pragma once
#include <cstdint>
#include <string>
#include <HubIDDataVer1.h>

namespace core {

class HubIDDataVer2 : public HubIDDataVer1 {
public:
    HubIDDataVer2() = delete;
    HubIDDataVer2( const char* buff, std::size_t flash_offset );
    HubIDDataVer2( const HubIDDataVer2& other ) = delete;
    HubIDDataVer2( HubIDDataVer2&& other ) = delete;
    HubIDDataVer2& operator=( const HubIDDataVer2& other ) = delete;
    HubIDDataVer2& operator=( HubIDDataVer2&& other ) = delete;
    ~HubIDDataVer2() = default;

    const char* name() const override;

    bool setId( std::uint64_t id );
    bool setUUID( const char* uuid, const size_t size );
    bool setZwaveRegion( const std::string& zwaveRegion );
    bool setDefaultWiFiSSID( const std::string& defaultWiFiSSID );
    bool setDefaultWiFiPass( const std::string& defaultWiFiPass );
    bool setProductName( const std::string& product_name );

public:
    static constexpr std::size_t NAME_OFFSET = 132;
    static constexpr std::size_t NAME_SIZE = 128;

protected:
    std::size_t m_flash_offset;
};

}  // namespace core
