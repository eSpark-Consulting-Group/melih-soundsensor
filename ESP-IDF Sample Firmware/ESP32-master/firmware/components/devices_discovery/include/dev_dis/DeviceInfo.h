#pragma once

#include "sdkconfig.h"

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

#include "dev_dis/Common.h"

#include <list>

namespace dev_dis {

class DeviceInfo {
public:
    static constexpr auto DefaultPrecision{ 30 };

    DeviceInfo()
    : m_precision{ DefaultPrecision } {}

    DeviceInfo( const Uuid& uuid, const std::string& name, const MacAddr& macAddr, const Precision precision );

    const auto& getId() const {
        return m_uuid;
    }

    const auto& getName() const {
        return m_name;
    }

    void setName( const std::string& name ) {
        m_name = name;
    }

    int getPrecision() const {
        return m_precision;
    }

    void setPrecision( const int precision ) {
        m_precision = precision;
    }

    const auto& getMac() const {
        return m_mac;
    }

    void setMac( const MacAddr& mac ) {
        m_mac = mac;
    }

    bool read( common::IReadStream& stream );

    bool write( common::IWriteStream& stream ) const;

private:
    Uuid m_uuid;
    std::string m_name;
    MacAddr m_mac;
    Precision m_precision;
};

using InfoStateList = std::list<std::tuple<DeviceInfo, std::optional<DevState>>>;

};  // namespace dev_dis

#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
