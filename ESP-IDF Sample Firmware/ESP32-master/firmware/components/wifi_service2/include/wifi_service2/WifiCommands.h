#pragma once

#include "BusCommand.h"
#include "WifiTypes.h"

namespace network {
namespace wifi_srv2 {
namespace responce {

class ConnectedToWifiStation : public bus::IBusCommand {
public:
    ConnectedToWifiStation() = default;

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

class DisconnectedFromWifiStation : public bus::IBusCommand {
public:
    DisconnectedFromWifiStation() = default;

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

class DiscoveredWifiAccessPoints : public bus::IBusCommand {
public:
    DiscoveredWifiAccessPoints()
    : m_isScanSuccess{ false } {};

    DiscoveredWifiAccessPoints( WifiApInfoSetType&& m_apInfoSet )
    : m_apSet( std::move( m_apInfoSet ) )
    , m_isScanSuccess{ true } {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    const auto& apInfo() const {
        return m_apSet;
    }

    auto isScanSuccess() const {
        return m_isScanSuccess;
    }

private:
    WifiApInfoSetType m_apSet;
    bool m_isScanSuccess;
};

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
class DiscoveredWifiBeacons : public bus::IBusCommand {
public:
    using Ssid = std::array<char, 34>;
    // Address to Bssid, Rssi, Ssid
    using AddrToBeaconInfo = std::map<BssidType, std::tuple<BssidType, Rssi, Ssid>>;

    DiscoveredWifiBeacons( AddrToBeaconInfo&& addrToInfo )
    : m_addrToInfo( std::move( addrToInfo ) ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline const auto& addrToInfo() const {
        return m_addrToInfo;
    }

private:
    AddrToBeaconInfo m_addrToInfo;
};
#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

}  // namespace responce
}  // namespace wifi_srv2
}  // namespace network
