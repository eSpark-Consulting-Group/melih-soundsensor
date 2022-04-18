#include "HubSettingsInfo.h"
#include "CommonResources.h"

namespace core {

std::map<std::string, int> HubSettingsInfo::DefaultCategoryDelays = {
    { "generic", 0 }, { "meter", 60 }, { "sensors", 10 }, { "security", 0 }, { "events", 10 },
};

TemperatureScaleEnum HubSettingsInfo::strToTemperatureScale( const char* str ) {
    if( !strcmp( str, scales::Celsius ) ) {
        return TemperatureScaleEnum::celsius;
    }
    else if( !strcmp( str, scales::Fahrenheit ) ) {
        return TemperatureScaleEnum::fahrenheit;
    }

    return TemperatureScaleEnum::undefinedTemperatureScale;
}

std::string HubSettingsInfo::temperatureScaleToStr( const TemperatureScaleEnum value ) {
    if( value == TemperatureScaleEnum::celsius ) {
        return scales::Celsius;
    }
    else if( value == TemperatureScaleEnum::fahrenheit ) {
        return scales::Fahrenheit;
    }

    return std::string();
}

TimeFormatEnum HubSettingsInfo::strToTimeFormat( const char* str ) {
    int val = (TimeFormatEnum)atoi( str );
    if( val != TimeFormatEnum::h12 && val != ( TimeFormatEnum::h24 ) ) {
        return TimeFormatEnum::undefinedTimeFormat;
    }

    return (TimeFormatEnum)val;
}

std::string HubSettingsInfo::timeFormatToStr( const TimeFormatEnum vaue ) {
    std::string str = std::to_string( (int)vaue );
    return str;
}

DateFormatEnum HubSettingsInfo::strToDateFormat( const char* str ) {
    if( !strcmp( str, tokens::DateFormatDMY ) ) {
        return DateFormatEnum::ddmmyy;
    }
    else if( !strcmp( str, tokens::DateFormatMDY ) ) {
        return DateFormatEnum::mmddyy;
    }

    return DateFormatEnum::undefinedDateFormat;
}

std::string HubSettingsInfo::dateFormatToStr( const DateFormatEnum value ) {
    if( value == DateFormatEnum::ddmmyy ) {
        return tokens::DateFormatDMY;
    }
    else if( value == DateFormatEnum::mmddyy ) {
        return tokens::DateFormatMDY;
    }

    return std::string();
}

std::vector<std::string> HubSettingsInfo::temperatureScaleValues() {
    return { scales::Celsius, scales::Fahrenheit };
}

std::vector<std::string> HubSettingsInfo::timeFormatValues() {
    return { "12", "24" };
}

std::vector<std::string> HubSettingsInfo::dateFormatValues() {
    return { tokens::DateFormatMDY, tokens::DateFormatDMY };
}

}  // namespace core
