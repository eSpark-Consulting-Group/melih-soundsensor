#include "SettingStorage.h"

#include "BinaryFileStorageFactory.h"
#include "StorageSection.h"
#include "ObjectSerializer.h"
#include "Cache.h"

using std::placeholders::_1;

static const char STATUS_KEY_PREFIX[] = "_status";
static const char VALUE_KEY_PREFIX[] = "_value";

static inline std::string statusKey( const std::string& settingId ) {
    return settingId + STATUS_KEY_PREFIX;
}

static inline std::string valueKey( const std::string& settingId ) {
    return settingId + VALUE_KEY_PREFIX;
}

static inline std::string infoKey( const std::string& settingId ) {
    return settingId;
}

static inline bool isStatusKey( const std::string& key ) {
    return key.size() > sizeof( STATUS_KEY_PREFIX ) - 1 &&  // size is at least prefix + 1 char
           key.rfind( STATUS_KEY_PREFIX ) == key.size() - sizeof( STATUS_KEY_PREFIX ) + 1;  // prefix is at the end
}

static inline bool isValueKey( const std::string& key ) {
    return key.size() > sizeof( VALUE_KEY_PREFIX ) - 1 &&  // size is at least prefix + 1 char
           key.rfind( VALUE_KEY_PREFIX ) == key.size() - sizeof( VALUE_KEY_PREFIX ) + 1;  // prefix is at the end
}

static inline bool isInfoKey( const std::string& id ) {
    return !isStatusKey( id ) && !isValueKey( id );
}

namespace storage {

Settings::Settings( KeyValueStorageFactory& storageFactory, const char* storageName )
: m_factory{ storageFactory }
, m_storage_name{ storageName } {}

bool Settings::setStatus( const std::string& settingId, Settings::SettingStatus newStatus ) {
    if( auto storage = m_factory.openNamespace( m_storage_name ) ) {
        return storage->setBlob(::statusKey( settingId ), std::vector<char>{ newStatus.begin(), newStatus.end() } );
    }
    return false;
}

std::optional<Settings::SettingStatus> Settings::getStatus( const std::string& settingId ) {
    if( auto storage = m_factory.openNamespace( m_storage_name ) ) {
        std::vector<char> blob;
        if( storage->getBlob(::statusKey( settingId ), blob ) ) {
            return std::string{ blob.data(), blob.size() };
        }
    }
    return std::nullopt;
}

bool Settings::setValue( const std::string& settingId, std::unique_ptr<::common::IValue> value ) {
    if( auto storage = m_factory.openNamespace( m_storage_name ) ) {
        return storage->set(::valueKey( settingId ), std::move( value ) );
    }
    return false;
}

std::unique_ptr<::common::IValue> Settings::getValue( const std::string& settingId ) {
    if( auto storage = m_factory.openNamespace( m_storage_name ) ) {
        std::vector<char> blob;
        if( auto value = storage->getIValue(::valueKey( settingId ) ) ) {
            return value;
        }
    }
    return {};
}

void Settings::remove( const std::string& settingId ) {
    if( auto storage = m_factory.openNamespace( m_storage_name ) ) {
        storage->remove(::infoKey( settingId ) );
        storage->remove(::statusKey( settingId ) );
        storage->remove(::valueKey( settingId ) );
    }
}

template <typename T>
std::unique_ptr<T> Settings::getInfo( const std::string& settingId ) {
    if( auto storage = m_factory.openNamespace( m_storage_name ) ) {
        std::vector<char> blob;
        if( storage->getBlob(::infoKey( settingId ), blob ) && !blob.empty() ) {
            auto result = std::make_unique<T>();
            result->setId( settingId );
            gateways::ObjectSerializer serializer;
            if( serializer.deserialize( std::move( blob ), *result ) ) {
                return result;
            }
        }
    }
    return {};
}

template <typename T>
bool Settings::setInfo( const T& settingsInfo, SettingStatus newStatus, std::unique_ptr<::common::IValue> value ) {
    if( auto storage = m_factory.openNamespace( m_storage_name ) ) {
        gateways::ObjectSerializer serializer;
        const auto& settingId = settingsInfo.getId();
        if( value ) {
            if( !storage->set(::valueKey( settingId ), std::move( value ) ) ) {
                return false;
            }
        }
        return storage->setBlob(::infoKey( settingId ), serializer.serialize( settingsInfo ) ) &&
               storage->setBlob(::statusKey( settingId ), std::vector<char>{ newStatus.begin(), newStatus.end() } );
    }
    return false;
}

template <typename T>
void Settings::enumerate( const std::function<bool( const T& )>& cb ) {
    if( auto storage = m_factory.openNamespace( m_storage_name ) ) {
        for( const auto& key : *storage ) {
            if( isInfoKey( key ) ) {
                if( auto info = getInfo<T>( key ); info && !cb( *info ) ) {
                    return;
                }
            }
        }
    }
}

void Settings::restoreLegacyValuesFromCache() {
    auto cb = []( Settings& storage, const auto& info ) {
        if( auto value = cache::Cache::get().getValueByKey( info.getId() ) ) {
            storage.setValue( info.getId(), std::move( value ) );
            cache::Cache::get().remove( info.getId() );
        }
        if( auto status = ::common::iValueCast<::common::StringValue>(
                cache::Cache::get().getValueByKey( info.getId() + common::resources::tokens::Status ) ) ) {
            storage.setStatus( info.getId(), status->get() );
            cache::Cache::get().remove( info.getId() + common::resources::tokens::Status );
        }
        return true;
    };

    ::storage::DeviceSettings::get().enumerate( std::bind( cb, std::ref(::storage::DeviceSettings::get() ), _1 ) );
    ::storage::GatewaySettings::get().enumerate( std::bind( cb, std::ref(::storage::GatewaySettings::get() ), _1 ) );
}

DeviceSettings& DeviceSettings::get() {
    static DeviceSettings s{ storage::BinaryFileStorageFactory::get(), section::SETTINGS };
    return s;
}

bool DeviceSettings::setInfo( const gateways::DeviceSettingsInfo& deviceSettingsInfo,
                              SettingStatus newStatus,
                              std::unique_ptr<::common::IValue> value ) {
    return Settings::setInfo( deviceSettingsInfo, std::move( newStatus ), std::move( value ) );
}

std::unique_ptr<gateways::DeviceSettingsInfo> DeviceSettings::getInfo( const std::string& settingId ) {
    return Settings::getInfo<gateways::DeviceSettingsInfo>( settingId );
}

void DeviceSettings::enumerate( const std::function<bool( const gateways::DeviceSettingsInfo& )>& cb ) {
    Settings::enumerate( cb );
}

GatewaySettings& GatewaySettings::get() {
    static GatewaySettings s{ storage::BinaryFileStorageFactory::get(), section::GATEWAY_SETTINGS };
    return s;
}

bool GatewaySettings::setInfo( const gateways::GatewaySettingsInfo& deviceSettingsInfo,
                               SettingStatus newStatus,
                               std::unique_ptr<::common::IValue> value ) {
    return Settings::setInfo( deviceSettingsInfo, std::move( newStatus ), std::move( value ) );
}

std::unique_ptr<gateways::GatewaySettingsInfo> GatewaySettings::getInfo( const std::string& settingId ) {
    return Settings::getInfo<gateways::GatewaySettingsInfo>( settingId );
}

void GatewaySettings::enumerate( const std::function<bool( const gateways::GatewaySettingsInfo& )>& cb ) {
    Settings::enumerate( cb );
}

}  // namespace storage
