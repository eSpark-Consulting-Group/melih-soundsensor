
#include <AbstractInfo.h>
#include <IValueCast.h>
#include <HouseModesManager.h>
#include <CommonResources.h>
#include <Cache.h>
#include <StorageFile.h>
#include <Storage.h>
#include <BinaryFileStorageFactory.h>
#include <StorageSection.h>
#include <DeviceInfo.h>
#include <ObjectSerializer.h>

#include <cmath>

#include <DebugUtils.h>

namespace gateways {

using namespace common::resources;

AbstractInfo::UPtr AbstractInfo::create( const StringType& deviceId ) {
    if( deviceId.empty() ) {
        return nullptr;
    }

    std::vector<char> buff;
    {
        auto devStor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ABSTRACTS );
        if( !devStor || !devStor->getBlob( deviceId, buff ) ) {
            return nullptr;
        }
    }

    AbstractInfo::UPtr deviceInfo = std::make_unique<AbstractInfo>();
    if( !deviceInfo ) {
        return nullptr;
    }

    return std::move( deviceInfo );
}

const char* AbstractInfo::getId() const {
    return m_deviceId.c_str();
}

const char* AbstractInfo::getCategory() const {
    return m_category.c_str();
}

const char* AbstractInfo::getSubcategory() const {
    return m_subcategory.c_str();
}

const char* AbstractInfo::getDeviceTypeId() const {
    return m_deviceTypeId.c_str();
}

const char* AbstractInfo::getName() const {
    return m_name.c_str();
}

const char* AbstractInfo::getType() const {
    return m_type.c_str();
}

const char* AbstractInfo::getSecurity() const {
    return m_security.c_str();
}

bool AbstractInfo::isBatteryPowered() const {
    return m_battery_powered;
}

bool AbstractInfo::isReachable() const {
    return m_reachable;
}

AbstractAccount AbstractInfo::getAccountInfo() {
    return m_info;
}

void AbstractInfo::setId( const char* id ) {
    m_deviceId = id;
}

void AbstractInfo::setCategory( const char* category ) {
    m_category = category;
}

void AbstractInfo::setSubcategory( const char* subCategory ) {
    m_subcategory = subCategory;
}

void AbstractInfo::setDeviceTypeId( const char* typeId ) {
    m_deviceTypeId.assign( typeId );
}

void AbstractInfo::setName( const char* name ) {
    m_name.assign( name );
}

void AbstractInfo::setType( const char* type ) {
    m_type.assign( type );
}

void AbstractInfo::setBatteryPowered( bool batteryPowered ) {
    m_battery_powered = batteryPowered;
}

void AbstractInfo::isReachable( bool reachable ) {
    m_reachable = reachable;
}

void AbstractInfo::setAccountInfo( AbstractAccount& accountInfo ) {
    m_info = accountInfo;
}

void AbstractInfo::setSecurity( const char* security ) {
    m_security.assign( security );
}

void AbstractInfo::setCapabilities( std::vector<std::string>& capabilities ) {
    m_capabilities = capabilities;
}

}  // namespace gateways
