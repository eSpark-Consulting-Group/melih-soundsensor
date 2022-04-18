#pragma once

#include "GatewayID.h"
#include "GatewayDefs.h"
#include "UpdatableService.h"

#include <memory>
#include <functional>

namespace storage {
class File;
}  // namespace storage

namespace common {
class IValue;
class TableValue;
}  // namespace common

namespace gateways {

class IGateway : public core::upd_srv::IUpdatable {
public:
    using ExecuteScriptResult = std::tuple<bool, std::unique_ptr<::common::IValue>>;
    using ScriptExecutor = std::function<ExecuteScriptResult( const std::string& )>;

    using ClbkRet = core::upd_srv::ClbkRet;
    using UpdContext = core::upd_srv::UpdContext;

    enum class AddingDeviceStatus { OK = 0, LeakOfResources, NotSupportedVersion, BrokenFileFormat };

    virtual GatewayID getTypeId() const = 0;

    virtual bool initialize() = 0;
    virtual void startup( ScriptExecutor ) = 0;

    virtual size_t getRecommendedThreadStackSize() const = 0;

    virtual AddingDeviceStatus restoreDeviceInfo( const DeviceId&, ::storage::File& ) = 0;
    virtual void cleanAllData() = 0;
};

IGateway* get( GatewayID id );

IGateway* getByName( const char* name );

IGateway* getByDevicePrefix( char prefix );

void enumerateGateways( std::function<bool( GatewayID, IGateway* )> clbk );

inline auto numberOfGateways() {
    return int( GatewayID::Undefined );
}

}  // namespace gateways
