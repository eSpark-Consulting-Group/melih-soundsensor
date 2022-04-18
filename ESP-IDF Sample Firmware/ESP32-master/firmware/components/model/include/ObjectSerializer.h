#pragma once

#include <DeviceInfo.h>
#include <SettingsInfo.h>
#include <vector>
#include <memory>
#include <string>
#include <map>
#include <SerializationBuffer.h>

#include "DeviceSettingsInfo.hpp"
#include "GatewaySettingsInfo.hpp"

namespace gateways {

class ObjectSerializer {
public:
    virtual std::vector<char> serialize( BaseDeviceInfo& object );
    virtual std::vector<char> serialize( ItemInfo& object );
    virtual std::vector<char> serialize( const DeviceSettingsInfo& object );
    virtual std::vector<char> serialize( const GatewaySettingsInfo& object );
    virtual bool deserialize( const std::string& id, std::vector<char>&& buff, BaseDeviceInfo& object );
    virtual bool deserialize( std::vector<char>&& buff, ItemInfo& object );
    virtual bool deserialize( std::vector<char>&& buff, DeviceSettingsInfo& object );
    virtual bool deserialize( std::vector<char>&& buff, GatewaySettingsInfo& object );

protected:
    template <typename T,
              typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, int>::type = 0>
    void write_item( T item );
    void write_string( const void* str, std::size_t length );
    void write_string( const std::string& str );
    void write_ivalue( const ::common::IValue& );

    bool read_item( int& value );
    bool read_item( bool& value );
    bool read_item( float& value );
    bool read_string( std::string& value );
    bool read_ivalue( std::unique_ptr<::common::IValue>& );

    void write_table( const std::map<std::string, std::string>& );
    bool read_table( std::map<std::string, std::string>& );

    void write_type_specific_fields( const ItemInfo& );
    void read_type_specific_fields( ItemInfo& );

    std::unique_ptr<common::SerializationBuffer> m_buff;
};

}  // namespace gateways
