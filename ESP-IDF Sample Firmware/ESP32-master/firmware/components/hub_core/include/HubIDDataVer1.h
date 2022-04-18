#pragma once
#include <HubIDData.h>

namespace core {

class HubIDDataVer1 : public HubIDData {
public:
    HubIDDataVer1() = delete;
    HubIDDataVer1( const char* buff );
    HubIDDataVer1( const HubIDDataVer1& other ) = delete;
    HubIDDataVer1( HubIDDataVer1&& other ) = delete;
    HubIDDataVer1& operator=( const HubIDDataVer1& other ) = delete;
    HubIDDataVer1& operator=( HubIDDataVer1&& other ) = delete;
    ~HubIDDataVer1() = default;

    std::uint64_t id() const override;
    const char* uuid() const override;
    const char* zwaveRegion() const override;
    const char* defaultWiFiSSID() const override;
    const char* defaultWiFiPass() const override;

    const char* name() const override;

public:
    static constexpr std::size_t ID_OFFSET = 4;
    static constexpr std::size_t ID_SIZE = 8;
    static constexpr std::size_t UUID_OFFSET = 12;
    static constexpr std::size_t UUID_SIZE = 16;
    static constexpr std::size_t ZWAVE_REGION_OFFSET = 28;
    static constexpr std::size_t ZWAVE_REGION_SIZE = 8;
    static constexpr std::size_t DEFAULT_WIFI_SSID_OFFSET = 36;
    static constexpr std::size_t DEFAULT_WIFI_SSID_MAX_SIZE = 32;
    static constexpr std::size_t DEFAULT_WIFI_PASS_OFFSET = 68;
    static constexpr std::size_t DEFAULT_WIFI_PASS_MAX_SIZE = 64;

protected:
    const char* m_buff;
};

}  // namespace core
