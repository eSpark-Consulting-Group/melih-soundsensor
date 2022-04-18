
#pragma once

#include <GatewayDefs.h>
#include <GatewayID.h>
#include <Utils.h>
#include <IValue.h>
#include <SerializationBuffer.h>

#include <memory>
#include <string>
#include <vector>
#include <tuple>
#include <functional>

namespace gateways {

struct AbstractAccount {
    std::string vendor;
    std::string account;
};

struct AbstractProtocol {
    std::string vendor;
};

class AbstractInfo {
public:
    using StringType = std::string;
    using UPtr = std::unique_ptr<AbstractInfo>;
    using Capatibilities = std::vector<std::string>;

    static UPtr create( const StringType& deviceId );
    const char* getId() const;
    const char* getCategory() const;
    const char* getSubcategory() const;
    const char* getDeviceTypeId() const;
    const char* getName() const;
    const char* getType() const;
    bool isBatteryPowered() const;
    bool isReachable() const;
    AbstractAccount getAccountInfo();
    const char* getSecurity() const;
    std::vector<std::string> getCapabilities() const;

    void setSecurity( const char* security );
    void setId( const char* id );
    void setCategory( const char* category );
    void setSubcategory( const char* subCategory );
    void setDeviceTypeId( const char* typeId );
    void setName( const char* name );
    void setType( const char* type );
    void setBatteryPowered( bool isBatteryPowered );
    void isReachable( bool reachable );
    void setAccountInfo( AbstractAccount& account );
    void setSecurity( std::string security );
    void setCapabilities( std::vector<std::string>& capabilities );

    std::string m_deviceId;
    std::string m_category;
    std::string m_subcategory;
    std::string m_deviceTypeId;
    AbstractProtocol* m_protocol;
    std::string m_name;
    std::string m_type;
    bool m_battery_powered;
    bool m_reachable;
    std::string m_reachable_reason;
    AbstractAccount m_info;
    std::string m_security;
    std::vector<std::string> m_capabilities;
};

}  // namespace gateways
