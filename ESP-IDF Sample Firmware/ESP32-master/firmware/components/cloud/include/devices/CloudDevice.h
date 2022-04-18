#pragma once
#ifdef CONFIG_CLOUD_DEVICES

#include <Storage.h>
#include <StorageFile.h>
#include <CloudDevice.h>
#include <stdint.h>
#include <vector>

namespace gateways {
namespace cloud {

struct Account {
    std::string vendor;
    std::string account;
};

struct Protocol {
    std::string vendor;
};

class Device {
public:
    using Capabilities = std::vector<std::string>;
    Device( const char* id );

public:
    void onDataUpdated();

    void resetDataUpdateTimestamp();

    bool needToUpdate() const;

    std::string getId() const;

    bool restoreData( storage::File& file );

    bool storeData( storage::File& file );

    void setName( const char* name );
    const char* getName() const;

    void setType( const char* typeId );
    const char* getType() const;

    void setTypeId( const char* typeId );
    const char* getTypeId() const;

    void setCategory( const char* category );
    const char* getCategory() const;

    void setSubCategory( const char* subcategory );
    const char* getSubCategory() const;

    void setBatteryPowered( bool powered );
    bool batteryPowered() const;

    void setDeviceId( const char* deviceId );
    const char* getDeviceId() const;

    void setSecurity( const char* deviceId );
    const char* getSecurity() const;

    void setProtocol( const Protocol& protocol );
    const Protocol& getProtocol() const;

    void setInfo( const Account& account );
    const Account& getInfo() const;

    void setReachable( bool reachable );
    bool isReachable() const;

    void setCapabilities( const Capabilities& capatibilityes );
    const Capabilities& getCapabilities();

private:
    bool readStorage( storage::File& file, std::string& data );

    bool writeProperty( storage::File& file, const char* name, const char* param );
    bool writeProperty( storage::File& file, const char* name, const bool param );
    bool writeProperty( storage::File& file, const char* name, std::string& param );
    bool writeProperty( storage::File& file, const char* name, const std::vector<std::string>& param );

    bool readProperty( const std::string& buf, const char* name, bool& param );
    bool readProperty( const std::string& buf, const char* name, std::vector<std::string>& param );
    bool readProperty( const std::string& buf, const char* name, std::string& param );

private:
    uint32_t m_lastDataUpdateTime = 0;
    std::string m_name;
    bool m_reachable;
    Account m_info;
    Protocol m_protocol;
    std::string m_deviceId;
    std::string m_type;
    std::string m_typeId;
    bool m_powered;
    std::string m_subCategory;
    bool m_batteryPowered;
    std::string m_deviceTypeId;
    std::string m_category;
    std::string m_security;
    Capabilities m_capabilities;
};

}  // namespace cloud
}  // namespace gateways

#endif  // CONFIG_CLOUD_DEVICES
