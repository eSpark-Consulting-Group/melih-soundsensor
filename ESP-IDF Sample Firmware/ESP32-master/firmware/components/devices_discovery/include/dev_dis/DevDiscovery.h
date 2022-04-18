#pragma once

// See documentation
// https://confluence.mios.com/display/ELFD/Devices+Detection

#include "sdkconfig.h"

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

#include "Shutdownable.h"
#include "dev_dis/DeviceInfo.h"

#include <mutex>
#include <map>

namespace dev_dis {

class DevDiscovery : public core::SingletonBaseImpl<DevDiscovery> {
    friend class core::SingletonBaseImpl<DevDiscovery>;

public:
    using ParentType = core::SingletonBaseImpl<DevDiscovery>;
    using ClbkRet = ParentType::ClbkRet;
    using UpdContext = core::upd_srv::UpdContext;

    Uuid addDevice( const char* name, const MacAddr& addr, const Precision precision );

    bool removeDevice( const Uuid& uuid );

    bool editDevice( const Uuid& uuid,
                     const std::optional<std::string>& name,
                     const std::optional<MacAddr>& addr,
                     const std::optional<Precision>& precision );

    InfoStateList getInfoStateList() const;

    std::optional<DeviceInfo> getInfo( const Uuid& uuid ) const;

    bool startScan();

    bool stopScan();

    bool startUnknowDevDetection();

    bool stopUnknowDevDetection();

private:
    using ParentType::SingletonBaseImpl;

    bool initializeImpl() override;

    ClbkRet updateComponent( UpdContext& ctx ) override;

    void onShutdownStart() override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const network::wifi_srv2::responce::DiscoveredWifiBeacons& cmd ) override;

    bool save( const DeviceInfo& deviceInfo ) const;

    bool saveAddrToState();

    bool saveUnknowAddrToState();

    bool load();

    void reset();

    bool startScanImpl();

    bool stopScanImpl();

private:
    mutable std::mutex m_mutex;

    unsigned int m_startCounter{ 0 }, m_unknowStartCounter{ 0 };

    std::map<Uuid, DeviceInfo> m_uuidToInfo;
    std::map<MacAddr, DeviceInfo&> m_addrToInfo;

    AddrToState m_addrToState;
    UuidToState m_uuidToStateLast, m_uuidToStateUpdNext;
    std::optional<DDTimePoint> m_addrToStateWasChangedTp{ std::nullopt };

    AddrToState m_unknowDev;
    AddrToState m_unknowDevUpd;
    std::optional<DDTimePoint> m_unknowAddrToStateWasChangedTp{ std::nullopt };
};

}  // namespace dev_dis

#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
