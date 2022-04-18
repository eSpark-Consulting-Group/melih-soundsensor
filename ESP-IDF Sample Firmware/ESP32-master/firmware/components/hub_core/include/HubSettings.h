#pragma once

#include "KeyValueStorageFactory.h"
#include "HubSettingsInfo.h"

#include "NmaBase.h"
#include <map>

namespace core {

class HubSettings {
public:
    static HubSettings& get();
    ~HubSettings() = default;

    TemperatureScaleEnum getTemperatureScale();
    void getTemperatureScale( TemperatureScaleEnum& value, std::string& type );
    bool setTemperatureScale( const TemperatureScaleEnum value );
    void resetTemperatureScale();

    void getTimeFormat( TimeFormatEnum& value, std::string& type );
    bool setTimeFormat( const TimeFormatEnum value );
    void resetTimeFormat();

    void getDateFormat( DateFormatEnum& value, std::string& type );
    bool setDateFormat( const DateFormatEnum value );
    void resetDateFormat();

    void getFirstStart( bool& val, std::string& type );
    void setFirstStart( const bool val );
    void resetFirstStart();

    bool setBroadcastDelayForCategory( const char* key, const int value );
    int getBroadcastDelayForCategory( const char* key );

    std::map<std::string, int> getBroadcastDelays();
    void resetBroadcastDelay();

    std::string getLocationUuid();
    bool setLocationUuid( std::string val );
    const char* getLocationUuidType();

    bool loadSettings();
    bool saveSettings();

private:
    HubSettings() {
        loadSettings();
    }
    HubSettingsInfo m_hubSettings;
    template <typename T>
    bool wasUpdatedValue( std::string name, T value );
};

}  // namespace core
