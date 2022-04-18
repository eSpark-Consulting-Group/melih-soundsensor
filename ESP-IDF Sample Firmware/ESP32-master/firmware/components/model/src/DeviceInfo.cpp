
#include <DeviceInfo.h>
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
#include "HubSettings.h"

#include "values/Int64Value.hpp"

#include <cmath>

#include <DebugUtils.h>

namespace gateways {

using namespace common::resources;

namespace {

constexpr int32_t BAD_USER_ID = -1;

class DeltaValueSetter : public common::IValueVisitor {
    float m_percent;
    ItemInfo::MinMaxType m_minmax;
    int m_errorCode = common::resources::errors::NotError;
    std::string m_errorMessage;

    template <typename T>
    void setValue( T& value ) {
        auto oldValue = value.get();
        float delta = m_percent * ( m_minmax.second - m_minmax.first ) / 100.0;
        if( ( float( oldValue ) >= m_minmax.second && 0.0 <= delta ) ||
            ( float( oldValue ) <= m_minmax.first && 0.0 >= delta ) ) {
            m_errorCode = common::resources::errors::InvalidParams;
            m_errorMessage = common::resources::errors::data::RangeInvalid;
            return;
        }

        decltype( oldValue ) newValue = oldValue + delta;
        // Correction "delta" rounding issues
        if( newValue == oldValue ) {
            newValue += std::copysign( 1, m_percent );
        }

        if( float( newValue ) > m_minmax.second ) {
            newValue = m_minmax.second;
        }
        else if( float( newValue ) < m_minmax.first ) {
            newValue = m_minmax.first;
        }
        value.set( newValue );
    }

public:
    DeltaValueSetter( const float percent, ItemInfo::MinMaxType minmax )
    : m_percent( percent )
    , m_minmax( minmax ) {}

    void visit( common::FloatValue& value ) override {
        setValue( value );
    };

    void visit( common::ScalebleValue& value ) override {
        setValue( value );
    };

    void visit( common::IntValue& value ) override {
        setValue( value );
    };

    void visit( common::Int64Value& value ) override {
        setValue( value );
    };

    void visit( common::BoolValue& value ) override {
        m_errorCode = common::resources::errors::InvalidParams;
        m_errorMessage = "DeltaValueSetter wasn't implemented for BoolValue!";
    }

    void visit( common::StringValue& value ) {
        m_errorCode = common::resources::errors::InvalidParams;
        m_errorMessage = "DeltaValueSetter wasn't implemented for StringValue!";
    }

    void visit( common::TokenValue& value ) {
        m_errorCode = common::resources::errors::InvalidParams;
        m_errorMessage = "DeltaValueSetter wasn't implemented for TokenValue!";
    }

    void visit( common::RgbValue& value ) {
        m_errorCode = common::resources::errors::InvalidParams;
        m_errorMessage = "DeltaValueSetter wasn't implemented for RgbValue!";
    }

    void visit( common::NotificationValue& value ) {
        m_errorCode = common::resources::errors::InvalidParams;
        m_errorMessage = "DeltaValueSetter wasn't implemented for NotificationValue!";
    }

    void visit( common::UserCodeValue& value ) {
        m_errorCode = common::resources::errors::InvalidParams;
        m_errorMessage = "DeltaValueSetter wasn't implemented for UserCodeValue!";
    }

    void visit( common::ArrayValue& value ) {
        m_errorCode = common::resources::errors::InvalidParams;
        m_errorMessage = "DeltaValueSetter wasn't implemented for ArrayValue!";
    }

    void visit( common::TableValue& value ) {
        m_errorCode = common::resources::errors::InvalidParams;
        m_errorMessage = "DeltaValueSetter wasn't implemented for TableValue!";
    }

    int errorCode() const {
        return m_errorCode;
    }

    std::string& errorMessage() {
        return m_errorMessage;
    }
};

}  // namespace

ItemInfo::UPtr ItemInfo::create( const ItemInfo::StringType& itemId ) {
    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ITEMS );
    if( !stor ) {
        return nullptr;
    }
    using BlobType = storage::KeyValueStorage::BlobType;
    BlobType blob;
    if( !stor->getBlob( itemId, blob ) || blob.empty() ) {
        return nullptr;
    }

    auto info = std::make_unique<gateways::ItemInfo>();
    gateways::ObjectSerializer serializer;
    if( !serializer.deserialize( std::move( blob ), *info ) ) {
        return nullptr;
    }

    info->setId( itemId );
    return std::move( info );
}

ItemInfo::ItemInfo( StringType deviceId,
                    StringType name,
                    StringType itemId,
                    ItemValueType valueType,
                    bool hasGetter,
                    bool hasSetter,
                    bool isVisible,
                    StringType userCodeRestriction,
                    StringType category,
                    Status status )
: BaseInfo( std::move( deviceId ), std::move( itemId ), valueType )
, m_name( std::move( name ) )
, m_hasGetter( hasGetter )
, m_hasSetter( hasSetter )
, m_isVisible( isVisible )
, m_userCodeRestriction( userCodeRestriction )
, m_category( category )
, m_defaultStatus( status ) {
    m_enums.reserve( 1 );
}

ItemInfo::ItemInfo( const ItemInfo& other )
: ItemInfo( other.getDeviceId(),
            other.getName(),
            other.getId(),
            other.getValueType(),
            other.hasGetter(),
            other.hasSetter(),
            other.isVisible(),
            other.getUserCodeRestriction(),
            other.getCategory(),
            stringToStatus( other.getStatus().c_str() ) ) {
    for( auto& field : other.getTypeSpecificFields() ) {
        setTypeSpecificField( field.first, field.second->clone() );
    }

    m_enums = other.m_enums;
}

void ItemInfo::Clear() {
    BaseInfo::Clear();
    m_name.clear();
    m_hasGetter = false;
    m_hasSetter = false;
    m_enums.clear();
    m_isVisible = true;
    m_userCodeRestriction.clear();
    m_typeSpecificFields.clear();
}

bool ItemInfo::checkEnumValue( const StringType& value ) const {
    for( const auto& enumVal : m_enums ) {
        if( enumVal == value ) {
            return true;
        }
    }
    return false;
}

bool ItemInfo::isUserLockOperation() const {
    return getName() == common::resources::tokens::UserLockOperation;
}

common::UserCodeValue::UPtr ItemInfo::getUserLockNameCode( const int codeIdx ) const {
    if( codeIdx == BAD_USER_ID ) {
        return nullptr;
    }
    char userIdStr[4] = { '\0' };
    snprintf( userIdStr, sizeof( userIdStr ), "%d", codeIdx );
    return getUserLockNameCode( userIdStr );
}

common::UserCodeValue::UPtr ItemInfo::getUserLockNameCode( const StringType& codeId ) const {
    if( codeId.empty() || !isUserLockOperation() ) {
        std::make_tuple( std::string(), std::string() );
    }

    common::UserCodeValue::UPtr userCode;
    const auto& deviceId = getDeviceId();
    gateways::enumerateItemsFromDb( [&userCode, &codeId, deviceId]( const gateways::ItemInfo& itemInfo ) {
        if( deviceId != itemInfo.getDeviceId() || itemInfo.getName() != common::resources::tokens::UserCodes ) {
            return true;
        }

        auto userCodeTbl = common::iValueCast<common::TableValue>( itemInfo.getValue() );
        if( userCodeTbl ) {
            userCode = common::iValueCast<common::UserCodeValue>( userCodeTbl->remove( codeId ) );
        }

        return false;
    } );

    return std::move( userCode );
}

bool BaseDeviceInfo::exists( const std::string& deviceId ) {
    auto storage = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DEVICES );
    if( !storage ) {
        return false;
    }
    return storage->exists( deviceId );
}

BaseDeviceInfo::UPtr BaseDeviceInfo::create( const StringType& deviceId ) {
    if( deviceId.empty() ) {
        return nullptr;
    }

    std::vector<char> buff;
    {
        auto devStor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DEVICES );
        if( !devStor || !devStor->getBlob( deviceId, buff ) ) {
            return nullptr;
        }
    }

    BaseDeviceInfo::UPtr deviceInfo = std::make_unique<BaseDeviceInfo>();
    if( !deviceInfo ) {
        return nullptr;
    }
    ObjectSerializer serializer;
    if( !serializer.deserialize( deviceId, std::move( buff ), *deviceInfo ) ) {
        return nullptr;
    }

    return std::move( deviceInfo );
}

const char* BaseDeviceInfo::getId() const {
    return m_deviceId.c_str();
}

const char* BaseDeviceInfo::getDeviceTypeId() const {
    return m_deviceTypeId.c_str();
}

const char* BaseDeviceInfo::getParentDeviceId() const {
    return m_parentDeviceId.c_str();
}

const char* BaseDeviceInfo::getName() const {
    return m_name.c_str();
}

const char* BaseDeviceInfo::getCategory() const {
    return m_category.c_str();
}

const char* BaseDeviceInfo::getSubcategory() const {
    return m_subCategory.c_str();
}

const char* BaseDeviceInfo::getGatewayId() const {
    return m_gatewayId.c_str();
}

const char* BaseDeviceInfo::getType() const {
    return m_type.c_str();
}

const char* BaseDeviceInfo::getSecurity() const {
    return m_security.c_str();
}

const char* BaseDeviceInfo::getStatus() const {
    return m_status.c_str();
}

bool BaseDeviceInfo::isBatteryPowered() const {
    return m_isBatteryPowered;
}

bool BaseDeviceInfo::isPersistent() const {
    return m_persistent;
}

std::map<std::string, std::string> BaseDeviceInfo::getInfo() const {
    return m_info;
}

std::map<std::string, std::string> BaseDeviceInfo::getFirmware() const {
    return m_firmware;
}

bool BaseDeviceInfo::isReachable() const {
    auto value = common::iValueCast<common::BoolValue>(
        cache::Cache::get().getValueByKey( m_deviceId + common::resources::tokens::Reachable ) );
    return value ? value->get() : false;
}

bool BaseDeviceInfo::isReady() const {
    auto value = common::iValueCast<common::BoolValue>(
        cache::Cache::get().getValueByKey( m_deviceId + common::resources::tokens::Ready ) );
    return value ? value->get() : true;  // Default value is true
}

bool BaseDeviceInfo::isReady( const ItemInfo::StringType devId ) {
    auto value = common::iValueCast<common::BoolValue>(
        cache::Cache::get().getValueByKey( devId + common::resources::tokens::Ready ) );
    return value ? value->get() : true;  // Default value is true
}

bool BaseDeviceInfo::isReadyByItemId( const ItemInfo::StringType itemId ) {
    auto itemsInfo = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ITEMS );
    if( !itemsInfo ) {
        FUNC_LOGE( "Failed to open store" );
        return false;
    }
    for( auto it = itemsInfo->cbegin(); it != itemsInfo->cend(); ++it ) {
        auto val = it.value();
        if( *it == itemId ) {
            if( !val.is_type<storage::KeyValueStorage::BlobType>() ||
                val.get<storage::KeyValueStorage::BlobType>().empty() ) {
                FUNC_LOGE( "Failed to get a current value for the key: %s", ( *it ).c_str() );
                continue;
            }
            ItemInfo itemInfo;
            ObjectSerializer serializer;
            if( !serializer.deserialize( std::move( val.get<storage::KeyValueStorage::BlobType>() ), itemInfo ) ) {
                FUNC_LOGE( "Failed to deserialize the key: %s", ( *it ).c_str() );
                continue;
            }
            return isReady( itemInfo.getDeviceId() );
        }
    }
    FUNC_LOGW( "Item '%s' not found", itemId.c_str() );
    return true;
}

bool BaseDeviceInfo::isArmed() const {
    return core::HouseModesManager::get().isDeviceArmed( m_deviceId );
}

const char* BaseDeviceInfo::getRoomId() const {
    return m_roomId.c_str();
}

void BaseDeviceInfo::setBatteryPowered( bool value ) {
    m_isBatteryPowered = value;
}

void BaseDeviceInfo::setPersistent( bool value ) {
    m_persistent = value;
}

void BaseDeviceInfo::setSecurity( const char* security ) {
    m_security.assign( security );
}

void BaseDeviceInfo::setStatus( const char* status ) {
    m_status.assign( status );
}

void BaseDeviceInfo::setInfo( const std::map<std::string, std::string> info ) {
    m_info = info;
}

void BaseDeviceInfo::setFirmware( const std::map<std::string, std::string> firmware ) {
    m_firmware = firmware;
}

std::vector<std::string> BaseDeviceInfo::getSettings() const {
    return {};
}

BaseDeviceInfo::StringType getDeviceIdByItemId( ItemInfo::StringType itemId ) {
    auto itemStor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ITEMS );
    if( !itemStor ) {
        return {};
    }

    for( auto it = itemStor->cbegin(); it != itemStor->cend(); ++it ) {
        auto val = it.value();
        if( *it != itemId ) {
            continue;
        }

        if( !val.is_type<storage::KeyValueStorage::BlobType>() ||
            val.get<storage::KeyValueStorage::BlobType>().empty() ) {
            FUNC_LOGE( "Failed to get a current value for the key: %s", ( *it ).c_str() );
            continue;
        }

        ItemInfo itemInfo;
        ObjectSerializer serializer;
        if( !serializer.deserialize( std::move( val.get<storage::KeyValueStorage::BlobType>() ), itemInfo ) ) {
            FUNC_LOGE( "Failed to deserialize the key: %s", ( *it ).c_str() );
            continue;
        }

        return itemInfo.getDeviceId();
    }

    return BaseDeviceInfo::StringType();
}

BaseDeviceInfo::UPtr getDeviceInfoByItemId( ItemInfo::StringType itemId ) {
    ItemInfo::StringType deviceId = getDeviceIdByItemId( itemId );
    return BaseDeviceInfo::create( deviceId );
}

void enumerateDevicesFromDb( std::function<bool( const BaseDeviceInfo& deviceInfo )> clbk ) {
    using BlobType = storage::KeyValueStorage::BlobType;

    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DEVICES );
    if( !stor ) {
        return;
    }

    gateways::BaseDeviceInfo deviceInfo;
    gateways::ObjectSerializer serializer;

    auto end = stor->cend();
    for( auto it = stor->cbegin(); end != it; ++it ) {
        deviceInfo.m_deviceId = *it;
        auto val = it.value();
        if( !val.is_type<BlobType>() || val.get<BlobType>().empty() ||
            !serializer.deserialize( *it, std::move( val.get<BlobType>() ), deviceInfo ) ) {
            return;
        }

        if( !clbk( deviceInfo ) ) {
            return;
        }
    }
}

void enumerateItemsFromDb( std::function<bool( const ItemInfo& itemInfo )> clbk ) {
    using BlobType = storage::KeyValueStorage::BlobType;

    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ITEMS );
    if( !stor ) {
        return;
    }

    gateways::ObjectSerializer serializer;

    auto end = stor->cend();
    for( auto it = stor->cbegin(); it != end; ++it ) {
        gateways::ItemInfo itemInfo;
        itemInfo.setId( *it );

        auto val = it.value();
        if( !val.is_type<BlobType>() || val.get<BlobType>().empty() ) {
            FUNC_LOGE( "Failed to get a current value for the key: %s", ( *it ).c_str() );
            continue;
        };

        if( !serializer.deserialize( std::move( val.get<BlobType>() ), itemInfo ) ) {
            return;
        }

        if( !clbk( itemInfo ) ) {
            return;
        }
    }
}

}  // namespace gateways
