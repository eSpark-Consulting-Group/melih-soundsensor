#ifdef CONFIG_CLOUD_DEVICES
#include <devices/CloudDevice.h>
#include <Utils.h>
#include <DebugUtils.h>

#include <StorageFile.h>

#include <iostream>
#include <sstream>
#include <iterator>

namespace gateways {
namespace cloud {

Device::Device( const char* id ) {
    m_deviceId = id;
}

void Device::onDataUpdated() {
    m_lastDataUpdateTime = common::getTimeSinceBootInSec();
}

void Device::resetDataUpdateTimestamp() {
    m_lastDataUpdateTime = 0;
}

std::string Device::getId() const {
    return m_deviceId;
}

bool Device::restoreData( storage::File& file ) {
    std::string buf;
    auto result = readStorage( file, buf );

    result = result && readProperty( buf, "id", m_deviceId );
    result = result && readProperty( buf, "name", m_name );
    result = result && readProperty( buf, "type", m_type );
    result = result && readProperty( buf, "typeid", m_typeId );
    result = result && readProperty( buf, "reachable", m_reachable );
    result = result && readProperty( buf, "info.vendor", m_info.vendor );
    result = result && readProperty( buf, "info.account", m_info.account );
    result = result && readProperty( buf, "protocol.vendor", m_protocol.vendor );
    result = result && readProperty( buf, "prowered", m_powered );
    result = result && readProperty( buf, "subcategory", m_subCategory );
    result = result && readProperty( buf, "category", m_category );
    result = result && readProperty( buf, "security", m_security );
    result = result && readProperty( buf, "capabilities", m_capabilities );

    return result;
}

bool Device::writeProperty( storage::File& file, const char* name, const char* param ) {
    std::string line = name;
    line += "=";
    line += param;
    line += "\n";
    return file.write( line.c_str(), line.size() );
}

bool Device::writeProperty( storage::File& file, const char* name, std::string& param ) {
    std::string line = name;
    line += "=";
    line += param.c_str();
    line += "\n";
    return file.write( line.c_str(), line.size() );
}

bool Device::writeProperty( storage::File& file, const char* name, const bool param ) {
    std::string line = name;
    line += "=";
    line += param ? "true" : "false";
    line += "\n";
    return file.write( line.c_str(), line.size() );
}

bool Device::writeProperty( storage::File& file, const char* name, const std::vector<std::string>& param ) {
    std::string line = name;
    line += "=";

    std::string str;
    for( auto item : param ) {
        str.append( item );
        str.append( "," );
    }

    line += str;
    line += "\n";
    return file.write( line.c_str(), line.size() );
}

bool Device::readStorage( storage::File& file, std::string& data ) {
    char buffer[512];
    file.read( (void*)buffer, 2 );
    int res = 0;
    do {
        res = file.read( (void*)buffer, 512 );
        data.append( buffer, 500 );
    } while( res );

    return true;
}

bool Device::readProperty( const std::string& buf, const char* name, std::string& param ) {
    std::istringstream stream( buf );
    std::string line;
    while( std::getline( stream, line ) ) {
        if( line.find( name ) == 0 ) {
            param = line.substr( line.find( "=" ) + 1 );
        }
    }
    return true;
}

bool Device::readProperty( const std::string& buf, const char* name, std::vector<std::string>& param ) {
    std::string str;

    if( readProperty( buf, name, str ) ) {
        std::stringstream ss( str );
        std::string tmp;
        while( std::getline( ss, tmp, ',' ) ) {
            param.push_back( tmp );
        }

        return true;
    }

    return false;
}

bool Device::readProperty( const std::string& buf, const char* name, bool& param ) {
    std::string res;
    if( readProperty( buf, name, res ) ) {
        param = ( res == "true" );
        return true;
    }
    return false;
}

bool Device::storeData( storage::File& file ) {
    auto result = writeProperty( file, "id", m_deviceId );
    result = result && writeProperty( file, "name", m_name );
    result = result && writeProperty( file, "type", m_type );
    result = result && writeProperty( file, "typeid", m_typeId );

    result = result && writeProperty( file, "reachable", m_reachable );
    result = result && writeProperty( file, "info.vendor", m_info.vendor.c_str() );
    result = result && writeProperty( file, "info.account", m_info.account.c_str() );
    result = result && writeProperty( file, "protocol.vendor", m_protocol.vendor.c_str() );
    result = result && writeProperty( file, "prowered", m_powered );
    result = result && writeProperty( file, "subcategory", m_subCategory );
    result = result && writeProperty( file, "category", m_category );
    result = result && writeProperty( file, "security", m_security );
    result = result && writeProperty( file, "capabilities", m_capabilities );
    result = result && file.write( 0 );
    return result;
}

void Device::setName( const char* name ) {
    m_name = name;
}

const char* Device::getName() const {
    return m_name.c_str();
}

void Device::setType( const char* type ) {
    m_type = type;
}

const char* Device::getType() const {
    return m_type.c_str();
}

void Device::setTypeId( const char* typeId ) {
    m_typeId = typeId;
}

const char* Device::getTypeId() const {
    return m_typeId.c_str();
}

void Device::setCategory( const char* category ) {
    m_category = category;
}

const char* Device::getCategory() const {
    return m_category.c_str();
}

void Device::setSubCategory( const char* subcategory ) {
    m_subCategory = subcategory;
}

const char* Device::getSubCategory() const {
    return m_subCategory.c_str();
}

void Device::setBatteryPowered( bool powered ) {
    m_powered = powered;
}

bool Device::batteryPowered() const {
    return m_powered;
}

void Device::setDeviceId( const char* deviceId ) {
    m_deviceId = deviceId;
}

const char* Device::getDeviceId() const {
    return m_deviceId.c_str();
}

void Device::setSecurity( const char* security ) {
    m_security = security;
}

const char* Device::getSecurity() const {
    return m_security.c_str();
}

void Device::setProtocol( const Protocol& protocol ) {
    m_protocol = protocol;
}

const Protocol& Device::getProtocol() const {
    return m_protocol;
}

void Device::setInfo( const Account& info ) {
    m_info = info;
}

const Account& Device::getInfo() const {
    return m_info;
}

void Device::setReachable( bool reachable ) {
    m_reachable = reachable;
}

bool Device::isReachable() const {
    return m_reachable;
}

const std::vector<std::string>& Device::getCapabilities() {
    return m_capabilities;
}

void Device::setCapabilities( const Capabilities& capabilities ) {
    m_capabilities = capabilities;
}

}  // namespace cloud
}  // namespace gateways
#endif  // CONFIG_CLOUD_DEVICES
