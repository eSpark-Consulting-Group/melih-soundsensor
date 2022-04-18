#pragma once

#include "Shutdownable.h"

namespace core {
namespace responces {
class TimeEstablished;
}  // namespace responces
}  // namespace core

namespace cloud_client {
namespace responces {
class ConnectedToCloud;
}  // namespace responces
}  // namespace cloud_client

namespace time_service {

class TimezoneService : public core::Shutdownable {
public:
    using ParentType = core::Shutdownable;
    using ClbkRet = ParentType::ClbkRet;
    using UpdContext = core::upd_srv::UpdContext;

    static constexpr auto TZ_DELAY = 150;
    static constexpr auto TZ_COUNTER = 200;

    using core::Shutdownable::Shutdownable;

    ClbkRet updateComponent( UpdContext& ctx ) override;

private:
    bool initializeImpl() override;

    void visit( const bus::BusAddr, const bus::BusAddr, command::SetTimezone& cmd ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, const responce::TimeEstablished& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const cloud_client::responces::ConnectedToCloud& command ) override {
        m_isConnectedToCloud = true;
    }

private:
    time_t getTimeToCheckExpired();
    void setExpireTime( time_t expTime );
    void setNextTimeAtMidnight();

private:
    unsigned long long tz_update_timepoint = 0;
    unsigned int cnt_get_tz = TZ_COUNTER;

    time_t m_expireTime = 0;
    time_t m_atmidnight = 0;
    uint8_t m_tzSetDelay = TZ_DELAY;
    bool m_isConnectedToCloud{ false };
};  // class TimezoneService

}  // namespace time_service
