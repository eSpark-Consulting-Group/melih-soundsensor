#pragma once

#include "dev_dis/Common.h"
#include "dev_dis/DeviceInfo.h"

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

#include "BusCommand.h"

namespace dev_dis {

namespace responce {

class DiscoveredDevices : public bus::IBusCommand {
public:
    DiscoveredDevices( UuidToState&& registered, AddrToState&& unknow )
    : m_registered{ std::move( registered ) }
    , m_unknow{ std::move( unknow ) } {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline const auto& registeredState() const {
        return m_registered;
    }

    inline const auto& unknowState() const {
        return m_unknow;
    }

private:
    UuidToState m_registered;
    AddrToState m_unknow;
};  // class DiscoveredDevices

class DeviceAdded : public bus::IBusCommand {
public:
    DeviceAdded( const DeviceInfo& devInfo )
    : m_devInfo{ devInfo } {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline const auto& deviceInfo() const {
        return m_devInfo;
    }

private:
    const DeviceInfo& m_devInfo;
};  // class DeviceAdded

class DeviceRemoved : public bus::IBusCommand {
public:
    DeviceRemoved( const Uuid& devId )
    : m_devId{ devId } {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline const auto& deviceId() const {
        return m_devId;
    }

private:
    Uuid m_devId;
};  // class DeviceRemoved

class DeviceEdited : public bus::IBusCommand {
    Uuid m_devId;
    MacAddr m_addr;
    std::optional<std::string> m_name;
    std::optional<Precision> m_precision;

public:
    DeviceEdited( const Uuid& devId, const MacAddr& addr )
    : m_devId{ devId }
    , m_addr{ addr } {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline const auto& deviceId() const {
        return m_devId;
    }

    const decltype( m_name )& name() const {
        return m_name;
    }

    inline void setName( const std::string& name ) {
        m_name = std::make_optional( name );
    }

    const decltype( m_addr )& addr() const {
        return m_addr;
    }

    const decltype( m_precision )& precision() const {
        return m_precision;
    }

    inline void setPrecision( const Precision& precision ) {
        m_precision = std::make_optional( precision );
    }
};  // class DeviceEdited

}  // namespace responce

}  // namespace dev_dis

#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
