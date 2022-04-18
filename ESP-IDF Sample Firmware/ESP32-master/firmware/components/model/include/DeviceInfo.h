
#pragma once

#include <GatewayDefs.h>
#include <GatewayID.h>
#include <Utils.h>
#include <SerializationBuffer.h>
#include <BaseInfo.h>

#include "IValue.h"
#include "IValueCast.h"
#include "values/UserCodeValue.h"

#include <memory>
#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <functional>
#include <unordered_map>
#include <MemCache.h>
#include "CommonResources.h"
#include <DebugUtils.h>

namespace common {
class IValue;
};

namespace gateways {

class ItemInfo : public BaseInfo {
    friend class ObjectSerializer;

public:
    using StringType = std::string;
    using EnumType = std::string;
    using EnumsArrayType = std::vector<EnumType>;
    using UPtr = std::unique_ptr<ItemInfo>;

    static UPtr create( const StringType& itemId );
    enum class Status : int { synced, outdated, unknown };
    static const char* statusToString( Status status ) {
        switch( status ) {
        case Status::synced:
            return common::resources::items::status::synced;
        case Status::outdated:
            return common::resources::items::status::outdated;
        default:
            return "";
        }
    }
    static Status stringToStatus( const char* status ) {
        if( strcmp( status, common::resources::items::status::synced ) == 0 ) {
            return Status::synced;
        }
        if( strcmp( status, common::resources::items::status::outdated ) == 0 ) {
            return Status::outdated;
        }
        return Status::unknown;
    }
    ItemInfo() = default;

    ItemInfo( StringType deviceId,
              StringType name,
              StringType itemId,
              ItemValueType valueType,
              bool hasGetter,
              bool hasSetter,
              bool isVisible,
              StringType userCodeRestriction,
              StringType category,
              Status status = Status::outdated );

    ItemInfo( const ItemInfo& other );

    void setDeviceId( const StringType& id ) {
        m_deviceId.assign( id );
    }

    const StringType& getDeviceId() const {
        return m_deviceId;
    }

    void setName( const StringType& name ) {
        m_name.assign( name );
    }

    const StringType& getName() const {
        return m_name;
    }

    void setHasGetter( bool getter ) {
        m_hasGetter = getter;
    }

    bool hasGetter() const {
        return m_hasGetter;
    }

    void setHasSetter( bool setter ) {
        m_hasSetter = setter;
    }

    bool hasSetter() const {
        return m_hasSetter;
    }

    bool isVisible() const {
        return m_isVisible;
    }

    void setCategory( const StringType& category ) {
        m_category.assign( category );
    }

    const StringType& getCategory() const {
        return m_category;
    }

    void setVisible( bool visible ) {
        m_isVisible = visible;
    }

    bool isEnum() const {
        return !m_enums.empty();
    }

    int getEnumItemsCount() const {
        return m_enums.size();
    }

    const StringType& getEnumValue( uint8_t idx ) const {
        return m_enums[idx];
    }

    std::string getStatus() const {
        auto status = cache::MemCache::get().getValueByKey( getId() + ::common::resources::tokens::Status );
        if( status ) {
            auto st = ::common::iValueCast<common::IntValue>( std::move( status ) );
            return statusToString( static_cast<Status>( st->get() ) );
        }
        return statusToString( m_defaultStatus );
    }

    void setEnumValues( EnumsArrayType&& values ) {
        m_enums = std::move( values );
    }

    void setStatus( const std::string& statusStr ) {
        if( auto status = stringToStatus( statusStr.c_str() ); status != Status::unknown ) {
            ::cache::MemCache::get().setValueByKey( getId() + ::common::resources::tokens::Status,
                                                    ::common::IntValue::create( static_cast<int>( status ) ) );
        }
    }

    bool checkEnumValue( const StringType& value ) const;

    void setUserCodeRestriction( const StringType& restriction ) {
        m_userCodeRestriction.assign( restriction );
    }

    const StringType& getUserCodeRestriction() const {
        return m_userCodeRestriction;
    }

    void Clear();

    bool isUserLockOperation() const;

    common::UserCodeValue::UPtr getUserLockNameCode( const int codeIdx ) const;

    common::UserCodeValue::UPtr getUserLockNameCode( const StringType& codeId ) const;

    void setTypeSpecificField( const std::string& name, std::unique_ptr<::common::IValue> field ) {
        m_typeSpecificFields[name] = std::move( field );
    }

    const std::unordered_map<std::string, std::unique_ptr<::common::IValue>>& getTypeSpecificFields() const {
        return m_typeSpecificFields;
    }

private:
    StringType m_name;
    bool m_hasGetter = false;
    bool m_hasSetter = false;
    EnumsArrayType m_enums;
    bool m_isVisible = true;
    StringType m_userCodeRestriction;
    std::unordered_map<std::string, std::unique_ptr<::common::IValue>> m_typeSpecificFields;
    StringType m_category = ::common::resources::tokens::General;
    Status m_defaultStatus;
};

class BaseDeviceInfo {
public:
    using StringType = std::string;
    using UPtr = std::unique_ptr<BaseDeviceInfo>;

    static bool exists( const std::string& deviceId );
    static UPtr create( const StringType& deviceId );

    const char* getId() const;

    const char* getDeviceTypeId() const;

    const char* getParentDeviceId() const;

    const char* getName() const;

    const char* getCategory() const;

    const char* getSubcategory() const;

    const char* getGatewayId() const;

    const char* getType() const;

    const char* getSecurity() const;

    const char* getStatus() const;

    bool isBatteryPowered() const;

    bool isPersistent() const;

    bool isReachable() const;

    bool isReady() const;

    static bool isReady( const ItemInfo::StringType devId );

    static bool isReadyByItemId( const ItemInfo::StringType itemId );

    bool isArmed() const;

    const char* getRoomId() const;

    std::vector<std::string> getSettings() const;

    std::map<std::string, std::string> getInfo() const;

    void setBatteryPowered( bool value );

    void setPersistent( bool value );

    void setSecurity( const char* security );

    void setStatus( const char* status );

    void setInfo( const std::map<std::string, std::string> );

    std::map<std::string, std::string> getFirmware() const;

    void setFirmware( const std::map<std::string, std::string> );

    inline bool isServiceNotification() const {
        return m_serviceNotification;
    }

    inline bool setServiceNotification( const bool serviceNotification ) {
        return m_serviceNotification = serviceNotification;
    }

    std::string m_gatewayId;
    std::string m_deviceId;
    std::string m_parentDeviceId;
    std::string m_deviceTypeId;
    std::string m_name;
    std::string m_category;
    std::string m_subCategory;
    std::string m_type;
    std::string m_roomId;
    std::string m_security;
    std::string m_status;
    std::map<std::string, std::string> m_info;
    std::map<std::string, std::string> m_firmware;

protected:
    bool m_isBatteryPowered;
    bool m_persistent = false;
    bool m_serviceNotification = false;
};

BaseDeviceInfo::StringType getDeviceIdByItemId( ItemInfo::StringType itemId );

BaseDeviceInfo::UPtr getDeviceInfoByItemId( ItemInfo::StringType itemId );

void enumerateDevicesFromDb( std::function<bool( const BaseDeviceInfo& deviceInfo )> clbk );

void enumerateItemsFromDb( std::function<bool( const ItemInfo& itemInfo )> clbk );

}  // namespace gateways
