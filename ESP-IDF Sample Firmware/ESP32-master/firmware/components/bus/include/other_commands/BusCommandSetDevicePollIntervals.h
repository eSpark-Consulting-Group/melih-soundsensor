#pragma once

#include <BusCommand.h>

namespace bus::command {

class SetDevicePollIntervals final : public bus::IBusCommand {
public:
    SetDevicePollIntervals( bool useFixedWakeUpInterval,
                            uint32_t wakeUpInterval,
                            uint32_t alwaysOnDevicesInetrval,
                            uint8_t attemptsCount )
    : m_useFixedWakeUpInterval( useFixedWakeUpInterval )
    , m_wakeUpInterval( wakeUpInterval )
    , m_alwaysOnDevicesInetrval( alwaysOnDevicesInetrval )
    , m_attemptsCount( attemptsCount ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline bool useFixedWakeUpInterval() const {
        return m_useFixedWakeUpInterval;
    }

    inline uint32_t wakeUpInterval() const {
        return m_wakeUpInterval;
    }

    inline uint32_t alwaysOnDevicesInetrval() const {
        return m_alwaysOnDevicesInetrval;
    }

    inline uint8_t attemptsCount() const {
        return m_attemptsCount;
    }

private:
    bool m_useFixedWakeUpInterval;
    uint32_t m_wakeUpInterval;
    uint32_t m_alwaysOnDevicesInetrval;
    uint8_t m_attemptsCount;
};

}  // namespace bus::command
