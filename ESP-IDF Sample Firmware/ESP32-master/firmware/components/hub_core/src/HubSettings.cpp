#include <cstring>

#include "HubSettings.h"
#include <BinaryFileStorageFactory.h>
#include <StorageSection.h>
#include "CommonResources.h"
#include <NmaBroadcastsSender.h>
#include "IRamBufStream.h"

namespace core {

using BufferStreamType = common::IRamBufferStream;
using namespace common::resources;

static constexpr const char* HubSettingsStorKey = "HubSettingsStorKey";

HubSettings& HubSettings::get() {
    static HubSettings res;
    return res;
}

bool HubSettings::loadSettings() {
    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::HUB_SETTINGS );
    storage::KeyValueStorage::BlobType blob;
    if( stor->getBlob( HubSettingsStorKey, blob ) ) {
        common::ArrayRStream stream( blob.data(), blob.size() );
        if( m_hubSettings.read( stream ) ) {
        }
        else {
            FUNC_LOGE( "Can't load hub settings from storage!" );
            return false;
        }
    }
    else {
        FUNC_LOGW( "Can't get hub settings blob from storage!" );
    }

    return true;
}

bool HubSettings::saveSettings() {
    BufferStreamType stream( BufferStreamType::MiddleBufSize );

    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::HUB_SETTINGS );

    bool is_successfull =
        stream && m_hubSettings.write( stream ) && stor->setBlob( HubSettingsStorKey, stream, stream.size() );
    if( is_successfull ) {
        FUNC_LOGI( "Save hub settings info to storage - done" );
    }
    else {
        FUNC_LOGE( "Save hub settings info to storage - error" );
    }

    return is_successfull;
}

bool valueIsContainsInEnum( std::list<std::string> list, std::string val ) {
    return std::find( list.begin(), list.end(), val ) != list.end();
}

TemperatureScaleEnum HubSettings::getTemperatureScale() {
    return m_hubSettings.temperatureScale();
}

void HubSettings::getTemperatureScale( TemperatureScaleEnum& value, std::string& type ) {
    loadSettings();
    value = m_hubSettings.temperatureScale();
    type = tokens::Token;
}

void HubSettings::resetTemperatureScale() {
    m_hubSettings.setTemperatureScale( TemperatureScaleEnum::fahrenheit );
}

bool HubSettings::setTemperatureScale( const TemperatureScaleEnum value ) {
    if( value == TemperatureScaleEnum::celsius || value == TemperatureScaleEnum::fahrenheit ) {
        if( value != m_hubSettings.temperatureScale() ) {
            m_hubSettings.setTemperatureScale( value );
            saveSettings();
            nma::BroadcastsSender::sendSettingStatusUpdateToken( tokens::ScaleTemperature,
                                                                 HubSettingsInfo::temperatureScaleToStr( value ) );
        }
        return true;
    }
    return false;
}

void HubSettings::getTimeFormat( TimeFormatEnum& value, std::string& type ) {
    loadSettings();
    value = m_hubSettings.timeFormatValue();
    type = tokens::Token;
}

bool HubSettings::setTimeFormat( const TimeFormatEnum value ) {
    if( value == TimeFormatEnum::h12 || value == TimeFormatEnum::h24 ) {
        if( value != m_hubSettings.timeFormatValue() ) {
            m_hubSettings.setTimeFormatValue( value );
            saveSettings();
            nma::BroadcastsSender::sendSettingStatusUpdateToken( tokens::TimeFormat,
                                                                 HubSettingsInfo::timeFormatToStr( value ) );
        }

        return true;
    }

    return false;
}

void HubSettings::resetTimeFormat() {
    m_hubSettings.setTimeFormatValue( TimeFormatEnum::h12 );
}

void HubSettings::getDateFormat( DateFormatEnum& value, std::string& type ) {
    loadSettings();
    value = m_hubSettings.dateFormatValue();
    type = tokens::Token;
}

bool HubSettings::setDateFormat( const DateFormatEnum value ) {
    if( value == DateFormatEnum::ddmmyy || value == DateFormatEnum::mmddyy ) {
        if( value != m_hubSettings.dateFormatValue() ) {
            m_hubSettings.setDateFormatValue( value );
            saveSettings();
            nma::BroadcastsSender::sendSettingStatusUpdateToken( tokens::DateFormat,
                                                                 HubSettingsInfo::dateFormatToStr( value ) );
        }

        return true;
    }

    return false;
}

void HubSettings::resetDateFormat() {
    m_hubSettings.setDateFormatValue( DateFormatEnum::mmddyy );
}

void HubSettings::getFirstStart( bool& value, std::string& type ) {
    loadSettings();
    value = m_hubSettings.firstStartValue();
    type = tokens::Bool;
}

void HubSettings::setFirstStart( const bool value ) {
    if( value != m_hubSettings.firstStartValue() ) {
        m_hubSettings.setFirstStartValue( value );
        saveSettings();
        nma::BroadcastsSender::sendSettingStatusUpdateFlag( tokens::FirstStart, value );
    }
}

void HubSettings::resetFirstStart() {
    m_hubSettings.setFirstStartValue( true );
}

std::string HubSettings::getLocationUuid() {
    loadSettings();
    return m_hubSettings.locationUuidValue();
}

const char* HubSettings::getLocationUuidType() {
    return tokens::String;
}

bool HubSettings::setLocationUuid( std::string value ) {
    if( value != m_hubSettings.locationUuidValue() ) {
        m_hubSettings.setLocationUuidValue( value );
        saveSettings();
        nma::BroadcastsSender::sendSettingStatusUpdateToken( tokens::LocationUUID, value );
    }

    return true;
}

bool HubSettings::setBroadcastDelayForCategory( const char* key, const int value ) {
    if( value < 0 || value > 3600 || ( key == tokens::Security && value > 0 ) ) {
        return false;
    }

    auto categoryDelays = m_hubSettings.categoryDelays();

    if( categoryDelays[key] != value ) {
        categoryDelays[key] = value;
        m_hubSettings.setCategoryDelays( categoryDelays );
        saveSettings();
        nma::BroadcastsSender::sendSettingStatusUpdateDictionary( tokens::BroadcastsDelay, key, value );
    }

    return true;
}

int HubSettings::getBroadcastDelayForCategory( const char* key ) {
    loadSettings();
    return m_hubSettings.categoryDelays()[key];
}

std::map<std::string, int> HubSettings::getBroadcastDelays() {
    loadSettings();
    return m_hubSettings.categoryDelays();
}

void HubSettings::resetBroadcastDelay() {
    m_hubSettings.setCategoryDelays( HubSettingsInfo::DefaultCategoryDelays );
}

}  // namespace core
