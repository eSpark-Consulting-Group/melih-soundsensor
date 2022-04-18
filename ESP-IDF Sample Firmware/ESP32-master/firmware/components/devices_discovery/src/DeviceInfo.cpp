#include "dev_dis/DeviceInfo.h"

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

#include "DebugUtils.h"

namespace dev_dis {

DeviceInfo::DeviceInfo( const Uuid& uuid, const std::string& name, const MacAddr& macAddr, const Precision precision )
: m_uuid{ uuid }
, m_name{ name }
, m_mac{ macAddr }
, m_precision{ precision } {}

bool DeviceInfo::write( common::IWriteStream& stream ) const {
    return stream.write( m_uuid ) && stream.write( m_name ) && stream.write( m_mac ) && stream.write( m_precision );
}

bool DeviceInfo::read( common::IReadStream& stream ) {
    return stream.read( m_uuid ) && stream.read( m_name ) && stream.read( m_mac ) && stream.read( m_precision );
}

std::string toDateString( const DDTimePoint& tp ) {
    char buffer[80];
    auto tm = DDClockType::to_time_t( tp );
    int res = std::strftime( buffer, sizeof( buffer ), "%Y-%m-%dT%H:%M:%S%z", std::gmtime( &tm ) );
    return res > 0 ? buffer : std::string{};
}

}  // namespace dev_dis

#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
