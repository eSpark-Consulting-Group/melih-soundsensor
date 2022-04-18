#pragma once

#include "Shutdownable.h"

#include <atomic>
#include <chrono>

namespace time_service {

class SntpService : public core::SingletonBaseImpl<SntpService> {
    friend class core::SingletonBaseImpl<SntpService>;

public:
    using ParentType = core::SingletonBaseImpl<SntpService>;
    using ClbkRet = ParentType::ClbkRet;
    using UpdContext = core::upd_srv::UpdContext;

    enum class SntpState { NotInitialized, WaitingWifi, WaitingSntpResp, SntpTimeEstablished };

    inline auto isTimeEstablished() const {
        return SntpState::SntpTimeEstablished == m_sntpState;
    }

private:
    using SingletonBaseImpl<SntpService>::SingletonBaseImpl;

    bool initializeImpl() override;

    void onShutdownStart() override;

    ClbkRet updateComponent( UpdContext& ctx ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const network::wifi_srv2::responce::ConnectedToWifiStation& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const network::wifi_srv2::responce::DisconnectedFromWifiStation& command ) override;

private:
    std::atomic<SntpState> m_sntpState = SntpState::NotInitialized;
};  // class SntpService

}  // namespace time_service
