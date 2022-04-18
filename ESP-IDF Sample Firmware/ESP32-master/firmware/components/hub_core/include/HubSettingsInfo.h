#pragma once

#include "IReadStream.h"
#include "IWriteStream.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "DebugUtils.h"
#include "CommonResources.h"
#include <string.h>
#include <map>

using namespace common::resources;

namespace core {

enum TemperatureScaleEnum { undefinedTemperatureScale, celsius, fahrenheit };
enum TimeFormatEnum { undefinedTimeFormat, h12 = 12, h24 = 24 };
enum DateFormatEnum { undefinedDateFormat, mmddyy, ddmmyy };

class HubSettingsInfo {
public:
    HubSettingsInfo() = default;

    static TemperatureScaleEnum strToTemperatureScale( const char* str );
    static std::string temperatureScaleToStr( const TemperatureScaleEnum value );
    static TimeFormatEnum strToTimeFormat( const char* str );
    static std::string timeFormatToStr( const TimeFormatEnum vaue );
    static DateFormatEnum strToDateFormat( const char* str );
    static std::string dateFormatToStr( const DateFormatEnum value );

    static std::vector<std::string> temperatureScaleValues();
    static std::vector<std::string> timeFormatValues();
    static std::vector<std::string> dateFormatValues();
    static std::map<std::string, int> categoryDelayValues();

    static std::map<std::string, int> DefaultCategoryDelays;

    bool read( common::IReadStream& stream ) {
        bool res = stream.read( m_temperatureScale ) && stream.read( m_timeFormat ) && stream.read( m_dateFormat ) &&
                   stream.read( m_locationUuid ) && stream.read( m_firstStart ) && stream.read( m_categoryDelays );
        return res;
    }

    bool write( common::IWriteStream& stream ) const {
        return stream.write( m_temperatureScale ) && stream.write( m_timeFormat ) && stream.write( m_dateFormat ) &&
               stream.write( m_locationUuid ) && stream.write( m_firstStart ) && stream.write( m_categoryDelays );
    }

    inline TemperatureScaleEnum temperatureScale() const {
        return m_temperatureScale;
    }

    inline void setTemperatureScale( TemperatureScaleEnum temperatureScale ) {
        if( m_temperatureScale != temperatureScale )
            m_temperatureScale = temperatureScale;
    }

    inline TimeFormatEnum timeFormatValue() const {
        return m_timeFormat;
    }

    inline void setTimeFormatValue( const TimeFormatEnum timeFormat ) {
        if( m_timeFormat != timeFormat )
            m_timeFormat = timeFormat;
    }

    inline DateFormatEnum dateFormatValue() const {
        return m_dateFormat;
    }

    inline void setCategoryDelays( std::map<std::string, int> categoryDelays ) {
        m_categoryDelays = categoryDelays;
    }

    inline std::map<std::string, int> categoryDelays() const {
        return m_categoryDelays;
    }

    inline void setDateFormatValue( const DateFormatEnum dateFormat ) {
        if( m_dateFormat != dateFormat )
            m_dateFormat = dateFormat;
    }

    inline bool firstStartValue() const {
        return m_firstStart;
    }

    inline void setFirstStartValue( const bool firstStart ) {
        if( m_firstStart != firstStart )
            m_firstStart = firstStart;
    }

    inline std::string locationUuidValue() const {
        return m_locationUuid;
    }

    inline void setLocationUuidValue( std::string& locationUuid ) {
        if( m_locationUuid != locationUuid )
            m_locationUuid = locationUuid;
    }

private:
    TemperatureScaleEnum m_temperatureScale = TemperatureScaleEnum::fahrenheit;
    TimeFormatEnum m_timeFormat = TimeFormatEnum::h12;
    DateFormatEnum m_dateFormat = DateFormatEnum::mmddyy;
    bool m_firstStart = true;
    std::string m_locationUuid;
    std::map<std::string, int> m_categoryDelays = DefaultCategoryDelays;
};

};  // namespace core
