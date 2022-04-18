#pragma once

#include "BusCommand.h"
#include "wifi_service2/WifiTypes.h"

#include <optional>

namespace network {
namespace wifi_srv2 {
namespace command {

class JoinToLastAccessPoint : public bus::IBusCommand {
public:
    JoinToLastAccessPoint() = default;

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

class JoinToAccessPoint : public bus::IBusCommand {
public:
    JoinToAccessPoint( std::string _ssid,
                       std::string _pass = std::string(),
                       const int _channel = 0,
                       std::optional<BssidType> _bssid = std::nullopt )
    : m_ssid( std::move( _ssid ) )
    , m_passw( std::move( _pass ) )
    , m_channel( _channel )
    , m_bssid( std::move( _bssid ) ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    const auto& ssid() const {
        return m_ssid;
    }

    const auto& passw() const {
        return m_passw;
    }

    const auto channel() const {
        return m_channel;
    }

    const auto& bssid() const {
        return m_bssid;
    }

private:
    std::string m_ssid, m_passw;
    int m_channel;
    std::optional<BssidType> m_bssid;
};

class DisjoinFromAccessPoint : public bus::IBusCommand {
public:
    DisjoinFromAccessPoint( const std::string& accessPointName = std::string{}, bool _isEraseAp = false )
    : m_accessPointName( accessPointName )
    , m_isEraseAp( _isEraseAp ){};

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    auto accessPointName() const {
        return m_accessPointName;
    }

    auto isEraseAp() const {
        return m_isEraseAp;
    }

private:
    std::string m_accessPointName;
    bool m_isEraseAp;
};

class StartAccessPoint : public bus::IBusCommand {
public:
    StartAccessPoint( const std::string& _ssid,
                      const std::string& _pass = std::string(),
                      int _channel = 1,
                      bool _isSsidHidden = false,
                      unsigned int _maxConnections = 4 )
    : m_ssid( std::move( _ssid ) )
    , m_passw( std::move( _pass ) )
    , m_channel( _channel )
    , m_isSsidHidden( _isSsidHidden )
    , m_maxConnections( _maxConnections ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    const auto& ssid() const {
        return m_ssid;
    }

    const auto& passw() const {
        return m_passw;
    }

    const auto channel() const {
        return m_channel;
    }

    auto isSsidHidden() const {
        return m_isSsidHidden;
    }

    auto maxConnections() const {
        return m_maxConnections;
    }

private:
    std::string m_ssid, m_passw;
    int m_channel;
    bool m_isSsidHidden;
    unsigned int m_maxConnections;
};

class StopAccessPoint : public bus::IBusCommand {
public:
    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

class StartWifiScan : public bus::IBusCommand {
public:
    StartWifiScan() = default;

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

class StopWifiScan : public bus::IBusCommand {
public:
    StopWifiScan() = default;

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

}  // namespace command
}  // namespace wifi_srv2
}  // namespace network
