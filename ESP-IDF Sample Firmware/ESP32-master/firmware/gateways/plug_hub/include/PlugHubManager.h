
#pragma once

#include <Bus.h>
#include <chrono>
#include <driver/gpio.h>

namespace gateways {
namespace plug_hub {

class PlugHubManager {
public:
    using ClockType = std::chrono::steady_clock;

    static PlugHubManager& get();

    bool initialize();

    bool onLoop();

    void onUpdatePlugState( const bool value );

    void onPressButton();

    inline bool getCurrrentPlugState() const {
        return m_plug_state;
    }

private:
    static constexpr auto STORE_PATH = "/vol/plug_hub.dat";
    static constexpr gpio_num_t RELAY_A_PIN = (gpio_num_t)CONFIG_IO_RELAY_A_PIN;
    static constexpr std::chrono::seconds UpdateMeterValue{ 20 };

    PlugHubManager() = default;

    int storeSave( size_t offset, const void* data, size_t size );
    int storeLoad( size_t offset, void* data, size_t size );
    int storeReset();

private:
    bool m_plug_state = false;

    ClockType::time_point m_lastDataUpdateTime = ClockType::now();
};

}  // namespace plug_hub
}  // namespace gateways
