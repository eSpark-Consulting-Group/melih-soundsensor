
#pragma once

#include <GatewayID.h>
#include <IGateway.h>

namespace gateways {
namespace water_valve {

class Gateway : public IGateway {
private:
    static constexpr uint8_t WATER_LEAK_SENSOR_COUNT = 1;

public:
    static Gateway& get();

    ~Gateway() override;

public:
    GatewayID getTypeId() const override;

    ClbkRet updateComponent( UpdContext& ctx ) override;

    size_t getRecommendedThreadStackSize() const override;

    bool initialize() override;

    void startup( ScriptExecutor ) override;

    AddingDeviceStatus restoreDeviceInfo( const DeviceId&, ::storage::File& ) override;
    void cleanAllData() override{};

    uint8_t getWaterLeakSensorCount() {
        return WATER_LEAK_SENSOR_COUNT;
    };

private:
    Gateway() = default;
};

}  // namespace water_valve
}  // namespace gateways
