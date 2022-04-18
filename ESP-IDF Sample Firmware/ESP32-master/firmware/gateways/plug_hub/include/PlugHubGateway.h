
#pragma once

#include <GatewayID.h>
#include <IGateway.h>

namespace gateways {
namespace plug_hub {

class Gateway : public IGateway {
public:
    static Gateway& get();

    ~Gateway() override;

public:
    GatewayID getTypeId() const override;

    ClbkRet updateComponent( UpdContext& ctx ) override;

    size_t getRecommendedThreadStackSize() const override;

    bool initialize() override;

    void startup( ScriptExecutor scriptExecutor ) override;

    AddingDeviceStatus restoreDeviceInfo( const DeviceId&, ::storage::File& ) override;
    void cleanAllData() override{};

private:
    Gateway() = default;
};

}  // namespace plug_hub
}  // namespace gateways
