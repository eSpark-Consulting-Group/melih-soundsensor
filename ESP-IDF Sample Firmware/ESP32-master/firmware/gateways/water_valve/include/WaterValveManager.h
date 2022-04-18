
#pragma once

#include <Bus.h>
#include <chrono>

namespace gateways {
namespace water_valve {

class WaterValveManager {
public:
    static WaterValveManager& get();

    bool initialize();
    bool onLoop();
    void onUpdateValveState( const bool value );
    void onPressButton();
    bool getCurrrentValveState();

protected:
    WaterValveManager() = default;
    bool m_valve_state = 0;
    const std::chrono::seconds UpdateWaterValveValue{ 20 };

    std::chrono::steady_clock::time_point m_lastDataUpdateTime = std::chrono::steady_clock::now();
};

}  // namespace water_valve
}  // namespace gateways
