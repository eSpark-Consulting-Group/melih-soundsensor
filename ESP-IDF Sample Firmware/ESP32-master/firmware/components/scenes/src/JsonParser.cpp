#include "JsonParser.h"

#include "conditions/Conditions.h"
#include "LocalScene.h"
#include "CommonResources.h"
#include "Utils.h"
#include "DeviceInfo.h"
#include "Time.h"

#include "IValueCast.h"

#include "Storage.h"
#include "FileStream.h"
#include "JsonStreamGenerator.h"

#include "gason.hpp"

#include <cstring>
#include <memory>
#include <cstdlib>
#include <cstdio>
#include <limits>

#include "DebugUtils.h"

namespace scenes {

using namespace common::resources;

//--------------------------------------------------------------------------------
// Forward declarations

std::tuple<int, std::string, Uuid> parseWhenBlock( const Uuid& parentUuid,
                                                   const gason::JsonValue& jsonRoot,
                                                   storage::KeyValueStorage& stor,
                                                   const WhenBlock::ParentType parentType );

//--------------------------------------------------------------------------------

void printJson( const gason::JsonValue& jsonRoot ) {
    if( !jsonRoot ) {
        FUNC_LOGW( "Empty json object" );
    }
    auto jsItr = gason::begin( jsonRoot );
    while( jsItr.isValid() ) {
        FUNC_LOGW( "js key = %s", jsItr->key );
        if( jsItr->value.isString() ) {
            FUNC_LOGW( "js value = %s", jsItr->value.toString() );
        }
        ++jsItr;
    }
}

common::IValue::UPtr parseIValue( const StringType& valueType, gason::JsonValue& jsonRoot ) {
    if( valueType.empty() || !jsonRoot ) {
        return nullptr;
    }

    auto val = common::makeIval( jsonRoot );
    if( valueType == values::RGB ) {
        return common::iValueCast<common::RgbValue>( std::move( val ) );
    }
    else if( valueType == values::String ) {
        return common::iValueCast<common::StringValue>( std::move( val ) );
    }
    else if( valueType == values::Token ) {
        return common::iValueCast<common::TokenValue>( std::move( val ) );
    }
    else if( valueType == values::Float ) {
        return common::iValueCast<common::FloatValue>( std::move( val ) );
    }
    else if( valueType == values::Bool ) {
        return common::iValueCast<common::BoolValue>( std::move( val ) );
    }
    else if( valueType == values::Int ) {
        return common::iValueCast<common::IntValue>( std::move( val ) );
    }
    else if( valueType == values::Usercode ) {
        return common::iValueCast<common::UserCodeValue>( std::move( val ) );
    }
    else if( valueType == tokens::Object ) {
        auto tblVal = common::iValueCast<common::TableValue>( val.get() );
        if( !tblVal ) {
            return nullptr;
        }
        return tblVal->remove( tokens::Value );
    }

    return std::move( val );
}

std::tuple<WhenBlockInterval::PeriodScale, DelayTypeSec, int, std::string> parseIntervalValue(
    gason::JsonValue& jsonRoot ) {
    WhenBlockInterval::PeriodScale scale = WhenBlockInterval::PeriodScale::Seconds;
    auto valueStr = jsonRoot.toString();
    if( !valueStr ) {
        return std::make_tuple( scale, DelayTypeSec( 0 ), errors::InvalidParams, "Value isn't string!" );
    }

    char* suffix;
    auto value = strtol( valueStr, &suffix, 10 );

    switch( *suffix ) {
    case 'd':
        scale = WhenBlockInterval::PeriodScale::Days;
        break;
    case 'h':
        scale = WhenBlockInterval::PeriodScale::Hours;
        break;
    case 'm':
        scale = WhenBlockInterval::PeriodScale::Minutes;
        break;
    case 's':
    default:
        break;
    }

    switch( *suffix ) {
    case 'd':
        value *= 24;
        [[fallthrough]];
    case 'h':
        value *= 60;
        [[fallthrough]];
    case 'm':
        value *= 60;
        [[fallthrough]];
    case 's':
        [[fallthrough]];
    default:
        break;
    }

    if( value ) {
        return std::make_tuple( scale, DelayTypeSec( value ), errors::NotError, "" );
    }
    else {
        return std::make_tuple( scale, DelayTypeSec( 0 ), errors::InvalidParams, "Wrong value string!" );
    }
}

std::tuple<int, int, std::string> parseIntType( const gason::JsonValue& jsonRoot,
                                                const int minVal = std::numeric_limits<int>::min(),
                                                const int maxVal = std::numeric_limits<int>::max() ) {
    bool isSuccess = false;
    auto value = jsonRoot.toInt( &isSuccess );
    if( !isSuccess ) {
        return std::make_tuple( 0, errors::ParseError, "Can't parse json value to int type!" );
    }

    if( value < minVal || value > maxVal ) {
        return std::make_tuple( 0, errors::ParseError, "Value out of bounds!" );
    }

    return std::make_tuple( value, errors::NotError, StringType{} );
}

std::tuple<Uint8Set, int, std::string> parseUint8Set( const gason::JsonValue& jsonRoot,
                                                      const unsigned int valMin,
                                                      const unsigned int valMax ) {
    if( !jsonRoot.isArray() ) {
        return std::make_tuple( Uint8Set(), errors::ParseError, "parseUint8Set: jsonRoot isn't array!" );
    }

    Uint8Set valueSet;
    for( auto jsItr = gason::begin( jsonRoot ); jsItr.isValid(); ++jsItr ) {
        auto [intVal, errCode, errDescr] = parseIntType( jsItr->value, valMin, valMax );
        if( errors::NotError != errCode ) {
            std::make_tuple( Uint8Set{}, errCode, std::move( errDescr ) );
        }

        valueSet.emplace( intVal );
    }

    return std::make_tuple( std::move( valueSet ), errors::NotError, StringType{} );
}

std::tuple<TimeOfDay, int, std::string> parse24HoursTime( const gason::JsonValue& jsonRoot ) {
    bool isSuccessfull;
    TimeOfDay timeOfDay;
    std::tie( isSuccessfull, timeOfDay ) = TimeInterval::create( jsonRoot.toString() );
    if( !isSuccessfull ) {
        return std::make_tuple( TimeOfDay{}, errors::ParseError, "parse24HoursTime: Can't parse string!" );
    }

    if( timeOfDay.valueSeconds() >= ( 24 * 60 * 60 ) ) {
        return std::make_tuple( TimeOfDay{}, errors::ParseError, "parse24HoursTime: Wrong hour or minute value!" );
    }

    return std::make_tuple( std::move( timeOfDay ), errors::NotError, StringType{} );
}

std::tuple<TimeOfDaySet, int, std::string> parse24HoursTimeArray( const gason::JsonValue& jsonRoot ) {
    if( !jsonRoot.isArray() ) {
        return std::make_tuple( TimeOfDaySet(), errors::ParseError, "parse24HoursTimeArray: jsonRoot isn't array!" );
    }

    TimeOfDaySet valueSet;
    for( auto jsItr = gason::begin( jsonRoot ); jsItr.isValid(); ++jsItr ) {
        auto [timeOfDay, errCode, errDescr] = parse24HoursTime( jsItr->value );
        if( errors::NotError != errCode ) {
            std::make_tuple( TimeOfDaySet{}, errCode, std::move( errDescr ) );
        }
        valueSet.emplace( std::move( timeOfDay ) );
    }

    return std::make_tuple( std::move( valueSet ), errors::NotError, StringType{} );
}

std::tuple<TimeInterval, int, std::string> parseHmsTimeInterval( const gason::JsonValue& jsonRoot ) {
    auto intStr = jsonRoot.toString();
    if( !intStr ) {
        return std::make_tuple(
            TimeInterval(), errors::ParseError, "parseHmsTimeInterval: Can't convert json value to string type!" );
    }

    bool isSuccessfull;
    TimeInterval interval;
    std::tie( isSuccessfull, interval ) = TimeInterval::create( intStr );
    return isSuccessfull ?
               std::make_tuple( interval, errors::NotError, StringType{} ) :
               std::make_tuple( TimeInterval(), errors::ParseError, "parseHmsTimeInterval: Can't parse string!" );
}

std::tuple<std::string, std::string, gason::JsonValue, int, std::string> parseNameTypeValue( gason::JsonValue& obj ) {
    std::string name, type;
    const auto strType = obj( tokens::Type ).toString();
    if( !strType ) {
        return std::make_tuple( std::move( name ),
                                std::move( type ),
                                gason::JsonValue(),
                                errors::InvalidParams,
                                "parseNameTypeValue: bad type!" );
    }
    type = strType;

    const auto strName = obj( tokens::Name ).toString();
    if( !strName ) {
        return std::make_tuple( std::move( name ),
                                std::move( type ),
                                gason::JsonValue(),
                                errors::InvalidParams,
                                "parseNameTypeValue: bad name!" );
    }
    name = strName;

    const auto jsValue = obj( tokens::Value );
    if( !jsValue ) {
        return std::make_tuple( std::move( name ),
                                std::move( type ),
                                gason::JsonValue(),
                                errors::InvalidParams,
                                "parseNameTypeValue: bad value!" );
    }

    return std::make_tuple( std::move( name ), std::move( type ), jsValue, errors::NotError, "" );
}

enum class SunModeIntervalType { None, Before, After, Intime };

std::tuple<SunModeIntervalType, int, std::string> parseSunMode( const gason::JsonValue& jsonRoot ) {
    const auto strType = jsonRoot.toString();
    if( !strType ) {
        return std::make_tuple(
            SunModeIntervalType::None, errors::ApplicationError, "parseDateMode: Value isn't string!" );
    }

    if( 0 == std::strcmp( strType, tokens::Before ) ) {
        return std::make_tuple( SunModeIntervalType::Before, errors::NotError, "" );
    }
    else if( 0 == std::strcmp( strType, tokens::After ) ) {
        return std::make_tuple( SunModeIntervalType::After, errors::NotError, "" );
    }
    else if( 0 == std::strcmp( strType, tokens::Intime ) ) {
        return std::make_tuple( SunModeIntervalType::Intime, errors::NotError, "" );
    }

    return std::make_tuple( SunModeIntervalType::None, errors::ApplicationError, "parseSunMode: Wrong value!" );
}

std::tuple<int, std::string, WhenBlockIsItemState::CompareMethod> parseComparator( gason::JsonValue jsValue ) {
    auto cmpCStr = jsValue.toString();
    if( !cmpCStr ) {
        return std::make_tuple(
            errors::ParseError, "Value is not string type!", WhenBlockIsItemState::CompareMethod::NotDefined );
    }
    StringType cmpMethodStr( cmpCStr );

    WhenBlockIsItemState::CompareMethod compareMethod = math::OperatorEqual == cmpMethodStr ?
                                                            WhenBlockIsItemState::CompareMethod::Equal :
                                                            math::OperatorLess == cmpMethodStr ?
                                                            WhenBlockIsItemState::CompareMethod::Less :
                                                            math::OperatorLessOrEqual == cmpMethodStr ?
                                                            WhenBlockIsItemState::CompareMethod::LessOrEqual :
                                                            math::OperatorMore == cmpMethodStr ?
                                                            WhenBlockIsItemState::CompareMethod::More :
                                                            math::OperatorMoreOrEqual == cmpMethodStr ?
                                                            WhenBlockIsItemState::CompareMethod::MoreOrEqual :
                                                            WhenBlockIsItemState::CompareMethod::NotDefined;

    if( WhenBlockIsItemState::CompareMethod::NotDefined == compareMethod ) {
        return std::make_tuple(
            errors::ParseError, "Bad compare method!", WhenBlockIsItemState::CompareMethod::NotDefined );
    }

    return std::make_tuple( errors::NotError, "", compareMethod );
}

std::tuple<std::set<std::string>, int, std::string> parseStringArrayToSetOfStrings( const gason::JsonValue& jsValue ) {
    if( !jsValue.isArray() ) {
        return std::make_tuple( std::set<std::string>{}, errors::InvalidParams, errors::data::BadParams );
    }

    std::set<std::string> res;
    for( auto jsItr = gason::begin( jsValue ); jsItr.isValid(); ++jsItr ) {
        auto cStr = jsItr->value.toString();
        if( !cStr ) {
            return std::make_tuple( std::move( res ), errors::InvalidParams, errors::data::BadParams );
        }
        res.emplace( cStr );
    }
    return std::make_tuple( std::move( res ), errors::NotError, StringType{} );
}

//--------------------------------------------------------------------------------

std::tuple<int, std::string> parseWhenBlockIsInterval( const Uuid& sceneUuid,
                                                       const Uuid& uuid,
                                                       const gason::JsonValue& fields,
                                                       storage::KeyValueStorage& stor,
                                                       const WhenBlock::ParentType parentType ) {
    int errorCode;
    StringType errorDescr;

    for( auto jsItr = gason::begin( fields ); jsItr.isValid(); ++jsItr ) {
        StringType name, type;
        gason::JsonValue jsValue;
        std::tie( name, type, jsValue, errorCode, errorDescr ) = parseNameTypeValue( jsItr->value );
        if( errors::NotError != errorCode ) {
            std::make_tuple( errorCode, std::move( errorDescr ) );
        }

        if( type == tokens::Interval && name == tokens::Interval ) {
            DelayTypeSec period;
            WhenBlockInterval::PeriodScale periodScale;
            std::tie( periodScale, period, errorCode, errorDescr ) = parseIntervalValue( jsValue );
            if( errors::NotError != errorCode ) {
                return std::make_tuple( errorCode, std::move( errorDescr ) );
            }

            WhenBlockInterval whBlock( uuid, parentType, sceneUuid, periodScale, period, currentTime() );
            if( !whBlock.save( stor ) ) {
                return std::make_tuple( errors::InternalError,
                                        "parseWhenBlockInterval: Can't save WhenBlockInterval to storage!" );
            }

            return std::make_tuple( errors::NotError, std::string() );
        }
    }

    return std::make_tuple( errors::InvalidParams, "parseWhenBlockInterval: bad fields!" );
}

std::tuple<int, std::string> parseWhenBlockIsDate( const Uuid& sceneUuid,
                                                   const Uuid& uuid,
                                                   const gason::JsonValue& fields,
                                                   storage::KeyValueStorage& stor,
                                                   const WhenBlock::ParentType parentType ) {
    int error = errors::NotError;
    StringType errorDescr;
    Uint8Set days, weekdays;
    TimeOfDaySet hours;

    for( auto jsItr = gason::begin( fields ); jsItr.isValid() && error == errors::NotError; ++jsItr ) {
        StringType name, type;
        gason::JsonValue jsValue;
        std::tie( name, type, jsValue, error, errorDescr ) = parseNameTypeValue( jsItr->value );
        if( errors::NotError != error ) {
            return std::make_tuple( error, errorDescr );
        }

        if( name == tokens::Weekdays ) {
            if( type == scenes_types::IntArray ) {
                auto valMin = 1, valMax = 7;
                std::tie( weekdays, error, errorDescr ) = parseUint8Set( jsValue, valMin, valMax );
            }
        }
        else if( name == tokens::Days ) {
            if( type == scenes_types::IntArray ) {
                auto valMin = 1, valMax = 31;
                std::tie( days, error, errorDescr ) = parseUint8Set( jsValue, valMin, valMax );
            }
        }
        else if( name == tokens::Time ) {
            if( type == scenes_types::HoursTimeArray ) {
                std::tie( hours, error, errorDescr ) = parse24HoursTimeArray( jsValue );
            }
        }
    }

    if( errors::NotError != error ) {
        return std::make_tuple( error, errorDescr );
    }

    WhenBlockIsDate whBlock(
        uuid, parentType, sceneUuid, std::move( weekdays ), std::move( days ), std::move( hours ) );
    return whBlock.save( stor ) ?
               std::make_tuple( errors::NotError, std::string() ) :
               std::make_tuple( errors::ParseError, "parseWhenBlockIsDate: Can't save WhenBlockIsDate to storage!" );
}

std::tuple<int, std::string> parseWhenBlockIsSunState( const Uuid& sceneUuid,
                                                       const Uuid& uuid,
                                                       const gason::JsonValue& fields,
                                                       storage::KeyValueStorage& stor,
                                                       const WhenBlock::ParentType parentType ) {
    int error = errors::NotError;
    std::string errorDescr;
    WhenBlockIsSunState::SunMode sunMode = WhenBlockIsSunState::SunMode::Sunrise;
    SunModeIntervalType intervalType = SunModeIntervalType::None;
    Uint8Set days, weekdays;
    TimeInterval interval;

    for( auto jsItr = gason::begin( fields ); jsItr.isValid() && error == errors::NotError; ++jsItr ) {
        StringType name, type;
        gason::JsonValue jsValue;
        std::tie( name, type, jsValue, error, errorDescr ) = parseNameTypeValue( jsItr->value );
        if( errors::NotError != error ) {
            return std::make_tuple( error, errorDescr );
        }

        if( name == tokens::Sunrise ) {
            std::tie( intervalType, error, errorDescr ) = parseSunMode( jsValue );
        }
        else if( name == tokens::Sunset ) {
            sunMode = WhenBlockIsSunState::SunMode::Sunset;
            std::tie( intervalType, error, errorDescr ) = parseSunMode( jsValue );
        }
        else if( name == tokens::Weekdays ) {
            if( type == scenes_types::IntArray ) {
                auto valMin = 1, valMax = 7;
                std::tie( weekdays, error, errorDescr ) = parseUint8Set( jsValue, valMin, valMax );
            }
        }
        else if( name == tokens::Days ) {
            if( type == scenes_types::IntArray ) {
                auto valMin = 1, valMax = 31;
                std::tie( days, error, errorDescr ) = parseUint8Set( jsValue, valMin, valMax );
            }
        }
        else if( name == tokens::Time ) {
            if( type == scenes_types::IntervalHms ) {
                std::tie( interval, error, errorDescr ) = parseHmsTimeInterval( jsValue );
            }
            else {
                FUNC_LOGW( "Time should be the \"hms_interval\" type!" );
            }
        }
    }

    if( errors::NotError != error ) {
        return std::make_tuple( error, errorDescr );
    }

    if( SunModeIntervalType::Before == intervalType ) {
        interval = -interval;
    }
    else if( SunModeIntervalType::Intime == intervalType ) {
        interval = TimeInterval();
    }
    else if( SunModeIntervalType::None == intervalType ) {
        return std::make_tuple( errors::ParseError, "Can't find type of sunstate!" );
    }

    WhenBlockIsSunState whBlock(
        uuid, parentType, sceneUuid, std::move( weekdays ), std::move( days ), sunMode, interval );
    return whBlock.save( stor ) ?
               std::make_tuple( errors::NotError, std::string() ) :
               std::make_tuple( errors::ParseError,
                                "parseWhenBlockIsSunState: Can't save WhenBlockIsSunState to storage!" );
}

std::tuple<int, std::string> parseWhenBlockIsOnce( const Uuid& sceneUuid,
                                                   const Uuid& uuid,
                                                   const gason::JsonValue& fields,
                                                   storage::KeyValueStorage& stor,
                                                   const WhenBlock::ParentType parentType ) {
    int error = errors::NotError;
    std::string errorDescr;
    TimeOfDay hour;
    WhenBlockIsOnce::DateType day = 0, month = 0, year = 0;

    for( auto jsItr = gason::begin( fields ); jsItr.isValid() && error == errors::NotError; ++jsItr ) {
        StringType name, type;
        gason::JsonValue jsValue;
        std::tie( name, type, jsValue, error, errorDescr ) = parseNameTypeValue( jsItr->value );
        if( errors::NotError != error ) {
            return std::make_tuple( error, errorDescr );
        }

        if( name == tokens::Day ) {
            if( type == scenes_types::Int ) {
                auto valMin = 1, valMax = 31;
                std::tie( day, error, errorDescr ) = parseIntType( jsValue, valMin, valMax );
            }
        }
        else if( name == tokens::Month ) {
            auto valMin = 1, valMax = 12;
            if( type == scenes_types::Int ) {
                std::tie( month, error, errorDescr ) = parseIntType( jsValue, valMin, valMax );
            }
        }
        else if( name == tokens::Year ) {
            if( type == scenes_types::Int ) {
                auto valMin = std::numeric_limits<int>::min(), valMax = std::numeric_limits<int>::max();
                std::tie( year, error, errorDescr ) = parseIntType( jsValue, valMin, valMax );
            }
        }
        else if( name == tokens::Time ) {
            if( type == scenes_types::HoursTime ) {
                std::tie( hour, error, errorDescr ) = parseHmsTimeInterval( jsValue );
            }
        }
    }

    if( errors::NotError != error ) {
        return std::make_tuple( error, errorDescr );
    }

    WhenBlockIsOnce whBlock( uuid, parentType, sceneUuid, day, month, year, std::move( hour ) );
    return whBlock.save( stor ) ?
               std::make_tuple( errors::NotError, std::string() ) :
               std::make_tuple( errors::ParseError, "parseWhenBlockIsOnce: Can't save WhenBlockIsOnce to storage!" );
}

std::tuple<int, std::string> parseWhenBlockIsItemState( const Uuid& sceneUuid,
                                                        const Uuid& uuid,
                                                        WhenBlockIsItemState::BlockType blockType,
                                                        const char* blockTypeName,
                                                        const gason::JsonValue& fields,
                                                        storage::KeyValueStorage& stor,
                                                        const WhenBlock::ParentType parentType ) {
    int error = errors::NotError;
    std::string errorDescr;
    std::string itemId;
    ValueTypeUPtr itemValue;
    WhenBlockIsItemState::ArmedState armedState = WhenBlockIsItemState::None;
    WhenBlockIsItemState::CompareMethod compareMethod = WhenBlockIsItemState::CompareMethod::Equal;
    ValueTypeUPtr compareOptions;

    for( auto jsItr = gason::begin( fields ); jsItr.isValid(); ++jsItr ) {
        StringType name, type;
        gason::JsonValue jsValue;
        std::tie( name, type, jsValue, error, errorDescr ) = parseNameTypeValue( jsItr->value );
        if( errors::NotError != error ) {
            return std::make_tuple( error, errorDescr );
        }

        if( name == tokens::Item ) {
            auto id = jsValue.toString();
            if( id ) {
                itemId = id;
            }
            else {
                return std::make_tuple( errors::ParseError, "Item name isn't string type!" );
            }
        }
        else if( name == tokens::Value ) {
            // #HACK: Added because iOS application sended invalid scenes json, i.e.
            // instead of "type" : "token" for "isButtonState" condition block iOS application
            // send invalid  "type" : "option" that leads parseIValue(...) method return wrong value (table instead of
            // string). This bug doesn't fixed for a long time and I decide to add this hack to fix creation
            // isButtonState scenes conditions from iOS app.
            if( WhenBlockIsItemState::BlockType::IsButtonState == blockType ) {
                if( values::Token != type ) {
                    FUNC_LOGW( "For \"isButtonState\" condition value type should be \"token\" but not %s",
                               type.c_str() );
                }
                itemValue = parseIValue( values::Token, jsItr->value );
            }
            else {
                itemValue = parseIValue( type, jsItr->value );
            }

            if( !itemValue ) {
                return std::make_tuple( errors::ParseError, "Can't convert \"value\" field into IValue type!" );
            }
        }
        else if( name == tokens::Armed ) {
            bool isSuccess;
            bool isArmed = jsValue.toBool( &isSuccess );
            if( isSuccess ) {
                armedState = isArmed ? WhenBlockIsItemState::IsArmed : WhenBlockIsItemState::IsDisarmed;
            }
            else {
                return std::make_tuple( errors::ParseError, "Can't convert \"value\" field into Bool type!" );
            }
        }
        else if( name == tokens::Comparator ) {
            std::tie( error, errorDescr, compareMethod ) = parseComparator( jsValue );
            if( errors::NotError != error ) {
                return std::make_tuple( error, errorDescr );
            }
        }
    }
    constexpr auto BUF_SIZE = 256;
    std::array<char, BUF_SIZE> buf;

    if( itemId.empty() ) {
        snprintf( buf.data(), BUF_SIZE, "Can't create itemId in %s block from json!", blockTypeName );
        return std::make_tuple( errors::ParseError, buf.data() );
    }

    if( !itemValue ) {
        snprintf( buf.data(), BUF_SIZE, "Can't create value in %s block from json!", blockTypeName );
        return std::make_tuple( errors::ParseError, buf.data() );
    }

    auto itemInfo = gateways::ItemInfo::create( itemId );
    if( itemInfo ) {
        itemValue = itemInfo->castToItemType( std::move( itemValue ) );
        if( !itemValue ) {
            snprintf( buf.data(),
                      BUF_SIZE,
                      "The type of value in %s block does not match value type of item!",
                      blockTypeName );
            return std::make_tuple( errors::ParseError, buf.data() );
        }
    }

    WhenBlockIsItemState whBlock( uuid,
                                  parentType,
                                  sceneUuid,
                                  blockType,
                                  std::move( itemId ),
                                  std::move( itemValue ),
                                  armedState,
                                  compareMethod );
    return whBlock.save( stor ) ?
               std::make_tuple( errors::NotError, std::string() ) :
               std::make_tuple( errors::ParseError,
                                "parseWhenBlockIsItemState: Can't save WhenBlockIsItemState to storage!" );
}

std::tuple<int, std::string> parseWhenBlockIsUserLockOperation( const Uuid& sceneUuid,
                                                                const Uuid& uuid,
                                                                const gason::JsonValue& fields,
                                                                storage::KeyValueStorage& stor,
                                                                const WhenBlock::ParentType parentType ) {
    int error = errors::NotError;
    std::string errorDescr;
    StringType itemId;
    WhenBlockIsUserLockOperation::StateType itemState;
    WhenBlockIsUserLockOperation::UserCodeType userCodeId = WhenBlockIsUserLockOperation::InvalidUserCodeId;

    for( auto jsItr = gason::begin( fields ); jsItr.isValid(); ++jsItr ) {
        StringType name, type;
        gason::JsonValue jsValue;
        std::tie( name, type, jsValue, error, errorDescr ) = parseNameTypeValue( jsItr->value );
        if( errors::NotError != error ) {
            return std::make_tuple( error, errorDescr );
        }

        if( name == tokens::Item ) {
            auto id = jsValue.toString();
            if( id ) {
                itemId = id;
            }
            else {
                return std::make_tuple( errors::ParseError, "Item name isn't string type!" );
            }
        }
        else if( name == tokens::State ) {
            auto state = jsValue.toString();
            if( state ) {
                itemState = state;
            }
            else {
                return std::make_tuple( errors::ParseError, "Item state isn't string type!" );
            }
        }
        else if( name == tokens::User ) {
            bool isSuccess;
            auto code = jsValue.toInt( &isSuccess );
            if( isSuccess ) {
                userCodeId = code;
            }
            else {
                auto codeCStr = jsValue.toString();
                if( codeCStr ) {
                    userCodeId = std::atoi( codeCStr );
                }
            }
        }
    }

    if( itemId.empty() ) {
        return std::make_tuple( errors::ParseError,
                                "Can't create \"Item ID\" in WhenBlockIsUserLockOperation block from json!" );
    }
    if( itemState.empty() ) {
        return std::make_tuple( errors::ParseError,
                                "Can't create \"Item State\" in WhenBlockIsUserLockOperation block from json!" );
    }
    if( WhenBlockIsUserLockOperation::InvalidUserCodeId == userCodeId ) {
        return std::make_tuple( errors::ParseError,
                                "Can't create \"User Code\" in WhenBlockIsUserLockOperation block from json!" );
    }

    WhenBlockIsUserLockOperation whBlock(
        uuid, parentType, sceneUuid, std::move( itemId ), std::move( itemState ), std::move( userCodeId ) );
    return whBlock.save( stor ) ?
               std::make_tuple( errors::NotError, std::string() ) :
               std::make_tuple(
                   errors::ParseError,
                   "parseWhenBlockIsUserLockOperation: Can't save WhenBlockIsUserLockOperation to storage!" );
}

std::tuple<int, std::string> parseWhenBlockHasAtLeastOneDictionaryValue( const Uuid& sceneUuid,
                                                                         const Uuid& uuid,
                                                                         const gason::JsonValue& fields,
                                                                         storage::KeyValueStorage& stor,
                                                                         const WhenBlock::ParentType parentType ) {
    int error = errors::NotError;
    std::string errorDescr;
    StringType itemId;
    ValueTypeUPtr dictValue;

    for( auto jsItr = gason::begin( fields ); jsItr.isValid(); ++jsItr ) {
        StringType name, type;
        gason::JsonValue jsValue;
        std::tie( name, type, jsValue, error, errorDescr ) = parseNameTypeValue( jsItr->value );
        if( errors::NotError != error ) {
            return std::make_tuple( error, errorDescr );
        }

        if( name == tokens::Item ) {
            auto id = jsValue.toString();
            if( id ) {
                itemId = id;
            }
            else {
                return std::make_tuple( errors::ParseError, "Item name isn't string type!" );
            }
        }
        else if( name == tokens::Value ) {
            dictValue = parseIValue( type, jsItr->value );
            if( !dictValue ) {
                return std::make_tuple( errors::ParseError, "Can't convert \"value\" field into IValue type!" );
            }
        }
    }

    if( itemId.empty() ) {
        return std::make_tuple( errors::ParseError,
                                "Can't create \"Item ID\" in WhenBlockHasAtLeastOneDictionaryValue block from json!" );
    }
    if( !dictValue ) {
        return std::make_tuple(
            errors::ParseError,
            "Can't create \"Item Value\" in WhenBlockHasAtLeastOneDictionaryValue block from json!" );
    }

    WhenBlockHasAtLeastOneDictionaryValue whBlock(
        uuid, parentType, sceneUuid, std::move( itemId ), std::move( dictValue ) );
    return whBlock.save( stor ) ?
               std::make_tuple( errors::NotError, std::string() ) :
               std::make_tuple( errors::ParseError,
                                "Can't save WhenBlockHasAtLeastOneDictionaryValue instance to storage!" );
}

std::tuple<int, std::string> parseWhenBlockIsDeviceDetected( const Uuid& sceneUuid,
                                                             const Uuid& uuid,
                                                             const gason::JsonValue& fields,
                                                             storage::KeyValueStorage& stor,
                                                             const WhenBlock::ParentType parentType,
                                                             const bool isUnknowDeviceDetected ) {
    int error = errors::NotError;
    std::string errorDescr;
    WhenBlockIsDeviceDetected::RssiType minRssi{ std::numeric_limits<decltype( minRssi )>::max() },
        maxRssi{ std::numeric_limits<decltype( maxRssi )>::min() };
    WhenBlockIsDeviceDetected::DevicesIds devicesIds;

    for( auto jsItr = gason::begin( fields ); jsItr.isValid(); ++jsItr ) {
        StringType name, type;
        gason::JsonValue jsValue;
        std::tie( name, type, jsValue, error, errorDescr ) = parseNameTypeValue( jsItr->value );
        if( errors::NotError != error ) {
            return std::make_tuple( error, errorDescr );
        }

        constexpr auto intMin = std::numeric_limits<int>::min();
        constexpr auto intMax = std::numeric_limits<int>::max();
        if( name == tokens::MinRssi ) {
            std::tie( minRssi, error, errorDescr ) = parseIntType( jsValue, intMin, intMax );
        }
        else if( name == tokens::MaxRssi ) {
            std::tie( maxRssi, error, errorDescr ) = parseIntType( jsValue, intMin, intMax );
        }
        else if( name == tokens::Device ) {
            std::tie( devicesIds, error, errorDescr ) = parseStringArrayToSetOfStrings( jsValue );
        }

        if( errors::NotError != error ) {
            return std::make_tuple( error, std::move( errorDescr ) );
        }
    }

    if( minRssi > maxRssi ) {
        return std::make_tuple( errors::InvalidParams, errors::data::BadParams );
    }

    if( isUnknowDeviceDetected ) {
        if( !devicesIds.empty() ) {
            return std::make_tuple( errors::InvalidParams, errors::data::BadParams );
        }

        WhenBlockIsUnknowDeviceDetected whBlock( uuid, parentType, sceneUuid, minRssi, maxRssi );
        return whBlock.save( stor ) ? std::make_tuple( errors::NotError, std::string() ) :
                                      std::make_tuple( errors::InternalError, errors::data::InternalOperationError );
    }
    else {
        if( devicesIds.empty() ) {
            return std::make_tuple( errors::InvalidParams, errors::data::BadParams );
        }

        WhenBlockIsDeviceDetected whBlock( uuid, parentType, sceneUuid, minRssi, maxRssi, std::move( devicesIds ) );
        return whBlock.save( stor ) ? std::make_tuple( errors::NotError, std::string() ) :
                                      std::make_tuple( errors::InternalError, errors::data::InternalOperationError );
    }
}

#ifdef CONFIG_ADVANCED_SCENES
std::tuple<int, std::string> parseWhenBlockAnd( const Uuid& sceneUuid,
                                                const Uuid& uuid,
                                                const gason::JsonValue& fields,
                                                storage::KeyValueStorage& stor,
                                                const WhenBlock::ParentType parentType ) {
    int error = errors::NotError;
    std::string errorDescr;
    UuidSet conditions;

    for( auto jsItr = gason::begin( fields ); jsItr.isValid() && error == errors::NotError; ++jsItr ) {
        StringType name, type;
        gason::JsonValue jsValue;
        std::tie( name, type, jsValue, error, errorDescr ) = parseNameTypeValue( jsItr->value );
        if( errors::NotError != error ) {
            return std::make_tuple( error, errorDescr );
        }

        if( name != tokens::Blocks || type != tokens::Blocks ) {
            continue;
        }

        UuidSet cnd;
        std::tie( error, errorDescr, cnd ) =
            parseWhenBlocksArray( sceneUuid, jsValue, stor, WhenBlock::ParentType::Condition );
        conditions.insert( cnd.begin(), cnd.end() );
    }

    if( errors::NotError != error ) {
        return std::make_tuple( error, errorDescr );
    }

    if( conditions.empty() ) {
        return std::make_tuple( errors::InvalidParams, StringType() );
    }

    WhenBlockAnd whBlock( uuid, parentType, sceneUuid, std::move( conditions ) );
    return whBlock.save( stor ) ?
               std::make_tuple( errors::NotError, std::string() ) :
               std::make_tuple( errors::ParseError, "parseWhenBlockAnd: Can't save parseWhenBlockAnd to storage!" );
}

std::tuple<int, std::string> parseWhenBlockOr( const Uuid& sceneUuid,
                                               const Uuid& uuid,
                                               const gason::JsonValue& fields,
                                               storage::KeyValueStorage& stor,
                                               const WhenBlock::ParentType parentType ) {
    int error = errors::NotError;
    std::string errorDescr;
    UuidSet conditions;

    for( auto jsItr = gason::begin( fields ); jsItr.isValid() && error == errors::NotError; ++jsItr ) {
        StringType name, type;
        gason::JsonValue jsValue;
        std::tie( name, type, jsValue, error, errorDescr ) = parseNameTypeValue( jsItr->value );
        if( errors::NotError != error ) {
            return std::make_tuple( error, errorDescr );
        }

        if( name != tokens::Blocks && type != tokens::Blocks ) {
            continue;
        }

        UuidSet cnd;
        std::tie( error, errorDescr, cnd ) =
            parseWhenBlocksArray( sceneUuid, jsValue, stor, WhenBlock::ParentType::Condition );
        conditions.insert( cnd.begin(), cnd.end() );
    }

    if( errors::NotError != error ) {
        return std::make_tuple( error, errorDescr );
    }

    if( conditions.empty() ) {
        return std::make_tuple( errors::InvalidParams, StringType() );
    }

    WhenBlockOr whBlock( uuid, parentType, sceneUuid, std::move( conditions ) );
    return whBlock.save( stor ) ? std::make_tuple( errors::NotError, std::string() ) :
                                  std::make_tuple( errors::ParseError, "parseWhenBlockOr: Can't save to storage!" );
}

std::tuple<int, std::string> parseWhenBlockNot( const Uuid& sceneUuid,
                                                const Uuid& uuid,
                                                const gason::JsonValue& fields,
                                                storage::KeyValueStorage& stor,
                                                const WhenBlock::ParentType parentType ) {
    int error = errors::NotError;
    std::string errorDescr;
    Uuid condition;

    for( auto jsItr = gason::begin( fields ); jsItr.isValid() && error == errors::NotError; ++jsItr ) {
        StringType name, type;
        gason::JsonValue jsValue;
        std::tie( name, type, jsValue, error, errorDescr ) = parseNameTypeValue( jsItr->value );
        if( errors::NotError != error ) {
            return std::make_tuple( error, errorDescr );
        }

        if( name != tokens::Block || type != tokens::Block ) {
            continue;
        }

        std::tie( error, errorDescr, condition ) =
            parseWhenBlock( sceneUuid, jsValue, stor, WhenBlock::ParentType::Condition );
        break;
    }

    if( errors::NotError != error ) {
        return std::make_tuple( error, errorDescr );
    }

    if( !condition ) {
        return std::make_tuple( errors::ParseError, "parseWhenBlockNot: Should contains 1 condition!" );
    }

    WhenBlockNot whBlock( uuid, parentType, sceneUuid, std::move( condition ) );
    return whBlock.save( stor ) ? std::make_tuple( errors::NotError, std::string() ) :
                                  std::make_tuple( errors::ParseError, "parseWhenBlockNot: Can't save to storage!" );
}

std::tuple<int, std::string> parseWhenBlockIsDateRange( const Uuid& sceneUuid,
                                                        const Uuid& uuid,
                                                        const gason::JsonValue& fields,
                                                        storage::KeyValueStorage& stor,
                                                        const WhenBlock::ParentType parentType ) {
    // https://confluence.mios.com/display/ELFD/2nd+Generation+of+Advanced+Scenes#id-2ndGenerationofAdvancedScenes-isDateRange
    int error = errors::NotError;
    StringType errorDescr;

    TimeOfDay startTime, endTime;
    DayOfMonth startDay, endDay;
    Month startMonth, endMonth;
    Year startYear, endYear;

    bool hasAtLeastOneStartBlock{ false }, hasAtLeastOneEndBlock{ false };

    for( auto jsItr = gason::begin( fields ); jsItr.isValid() && error == errors::NotError; ++jsItr ) {
        StringType name, type;
        gason::JsonValue jsValue;
        std::tie( name, type, jsValue, error, errorDescr ) = parseNameTypeValue( jsItr->value );
        if( errors::NotError != error ) {
            return std::make_tuple( error, errorDescr );
        }

        if( type == scenes_types::HoursTimeArray ) {
            if( name == tokens::StartTime ) {
                std::tie( startTime, error, errorDescr ) = parse24HoursTime( jsValue );
                if( errors::NotError == error ) {
                    hasAtLeastOneStartBlock = true;
                }
            }
            else if( name == tokens::EndTime ) {
                std::tie( endTime, error, errorDescr ) = parse24HoursTime( jsValue );
                if( errors::NotError == error ) {
                    hasAtLeastOneEndBlock = true;
                }
            }
        }
        else if( type == scenes_types::Int ) {
            constexpr auto minDay = 1, maxDay = 31;
            if( name == lua_events::StartDay ) {
                std::tie( startDay, error, errorDescr ) = parseIntType( jsValue, minDay, maxDay );
                if( errors::NotError == error ) {
                    hasAtLeastOneStartBlock = true;
                }
            }
            else if( name == tokens::EndDay ) {
                std::tie( endDay, error, errorDescr ) = parseIntType( jsValue, minDay, maxDay );
                if( errors::NotError == error ) {
                    hasAtLeastOneEndBlock = true;
                }
            }
            else if( name == lua_events::StartMonth ) {
                std::tie( startMonth, error, errorDescr ) = parseIntType( jsValue, Month::Jan, Month::Dec );
                if( errors::NotError == error ) {
                    hasAtLeastOneStartBlock = true;
                }
            }
            else if( name == tokens::EndMonth ) {
                std::tie( endMonth, error, errorDescr ) = parseIntType( jsValue, Month::Jan, Month::Dec );
                if( errors::NotError == error ) {
                    hasAtLeastOneEndBlock = true;
                }
            }
            else if( name == lua_events::StartYear ) {
                constexpr auto minYear = 1;
                std::tie( startYear, error, errorDescr ) = parseIntType( jsValue, minYear );
                if( errors::NotError == error ) {
                    hasAtLeastOneStartBlock = true;
                }
            }
            else if( name == tokens::EndYear ) {
                constexpr auto minYear = 1;
                std::tie( endYear, error, errorDescr ) = parseIntType( jsValue, minYear );
                if( errors::NotError == error ) {
                    hasAtLeastOneEndBlock = true;
                }
            }
        }
    }

    if( errors::NotError != error ) {
        return std::make_tuple( error, errorDescr );
    }

    if( !hasAtLeastOneStartBlock && !hasAtLeastOneEndBlock ) {
        return std::make_tuple( errors::ParseError, "parseWhenBlockIsDateRange: Wrong condition!" );
    }

    WhenBlockIsDateRange whBlock( uuid,
                                  parentType,
                                  sceneUuid,
                                  std::move( startTime ),
                                  std::move( endTime ),
                                  std::move( startDay ),
                                  std::move( endDay ),
                                  std::move( startMonth ),
                                  std::move( endMonth ),
                                  std::move( startYear ),
                                  std::move( endYear ) );
    return whBlock.save( stor ) ?
               std::make_tuple( errors::NotError, std::string() ) :
               std::make_tuple( errors::ParseError,
                                "parseWhenBlockIsDateRange: Can't save WhenBlockIsDateRange to storage!" );
}
#endif  // CONFIG_ADVANCED_SCENES

std::tuple<int, std::string> parseWhenBlockEqual( const Uuid& sceneUuid,
                                                  const Uuid& uuid,
                                                  const gason::JsonValue& fields,
                                                  storage::KeyValueStorage& stor,
                                                  const WhenBlock::ParentType parentType ) {
    int error = errors::NotError;
    std::string errorDescr;
    UuidSet conditions;

    for( auto jsItr = gason::begin( fields ); jsItr.isValid() && error == errors::NotError; ++jsItr ) {
        StringType name, type;
        gason::JsonValue jsValue;
        std::tie( name, type, jsValue, error, errorDescr ) = parseNameTypeValue( jsItr->value );
        if( errors::NotError != error ) {
            return std::make_tuple( error, errorDescr );
        }

        if( name != tokens::LeftBlock || name != tokens::LeftBlock || type != tokens::Block ) {
            continue;
        }

        Uuid blockUuid;
        std::tie( error, errorDescr, blockUuid ) =
            parseWhenBlock( sceneUuid, jsValue, stor, WhenBlock::ParentType::Condition );
        if( errors::NotError != error ) {
            break;
        }

        if( blockUuid ) {
            conditions.emplace( std::move( blockUuid ) );
        }
    }

    if( errors::NotError != error ) {
        return std::make_tuple( error, errorDescr );
    }

    if( conditions.size() < 2 ) {
        return std::make_tuple( errors::ParseError, "parseWhenBlockEqual: Should contains at least 2 conditions!" );
    }

    WhenBlockEqual whBlock( uuid, parentType, sceneUuid, std::move( conditions ) );
    return whBlock.save( stor ) ? std::make_tuple( errors::NotError, std::string() ) :
                                  std::make_tuple( errors::ParseError, "parseWhenBlockEqual: Can't save to storage!" );
}

std::tuple<int, std::string, Uuid> parseWhenBlock( const Uuid& parentUuid,
                                                   const gason::JsonValue& jsonRoot,
                                                   storage::KeyValueStorage& stor,
                                                   const WhenBlock::ParentType parentType ) {
    if( !jsonRoot.isObject() ) {
        return std::make_tuple( errors::InvalidParams, "parseWhenBlock: jsonRoot isn't json object", Uuid() );
    }

    const auto methodName = jsonRoot( tokens::BlockOptions )( tokens::Method )( tokens::Name ).toString();
    if( !methodName ) {
        return std::make_tuple( errors::InvalidParams, "parseWhenBlock: method name isn't exist", Uuid() );
    }

    const auto fields = jsonRoot( tokens::Fields );
    if( !fields.isArray() ) {
        return std::make_tuple( errors::ParseError, "parseWhenBlock: \"fields\" isn't json array!", Uuid() );
    }

    auto blockUuid = Uuid::generate();
    int error;
    std::string errorDescr;
    if( 0 == std::strcmp( methodName, method_names::IsInterval ) ) {
        std::tie( error, errorDescr ) = parseWhenBlockIsInterval( parentUuid, blockUuid, fields, stor, parentType );
    }
    else if( 0 == std::strcmp( methodName, method_names::IsDate ) ) {
        std::tie( error, errorDescr ) = parseWhenBlockIsDate( parentUuid, blockUuid, fields, stor, parentType );
    }
    else if( 0 == std::strcmp( methodName, method_names::IsSunState ) ) {
        std::tie( error, errorDescr ) = parseWhenBlockIsSunState( parentUuid, blockUuid, fields, stor, parentType );
    }
    else if( 0 == std::strcmp( methodName, method_names::IsOnce ) ) {
        std::tie( error, errorDescr ) = parseWhenBlockIsOnce( parentUuid, blockUuid, fields, stor, parentType );
    }
    else if( 0 == std::strcmp( methodName, method_names::IsItemState ) ) {
        std::tie( error, errorDescr ) = parseWhenBlockIsItemState( parentUuid,
                                                                   blockUuid,
                                                                   WhenBlockIsItemState::BlockType::ItemState,
                                                                   method_names::IsItemState,
                                                                   fields,
                                                                   stor,
                                                                   parentType );
    }
    else if( 0 == std::strcmp( methodName, method_names::IsCompareNumber ) ) {
        std::tie( error, errorDescr ) = parseWhenBlockIsItemState( parentUuid,
                                                                   blockUuid,
                                                                   WhenBlockIsItemState::BlockType::CompareNumber,
                                                                   method_names::IsCompareNumber,
                                                                   fields,
                                                                   stor,
                                                                   parentType );
    }
    else if( 0 == std::strcmp( methodName, method_names::IsButtonState ) ) {
        std::tie( error, errorDescr ) = parseWhenBlockIsItemState( parentUuid,
                                                                   blockUuid,
                                                                   WhenBlockIsItemState::BlockType::IsButtonState,
                                                                   method_names::IsButtonState,
                                                                   fields,
                                                                   stor,
                                                                   parentType );
    }
#ifdef CONFIG_ADVANCED_SCENES
    else if( 0 == std::strcmp( methodName, method_names::And ) ) {
        std::tie( error, errorDescr ) = parseWhenBlockAnd( parentUuid, blockUuid, fields, stor, parentType );
    }
    else if( 0 == std::strcmp( methodName, method_names::Or ) ) {
        std::tie( error, errorDescr ) = parseWhenBlockOr( parentUuid, blockUuid, fields, stor, parentType );
    }
    else if( 0 == std::strcmp( methodName, method_names::Not ) ) {
        std::tie( error, errorDescr ) = parseWhenBlockNot( parentUuid, blockUuid, fields, stor, parentType );
    }
    else if( 0 == std::strcmp( methodName, method_names::IsDateRange ) ) {
        std::tie( error, errorDescr ) = parseWhenBlockIsDateRange( parentUuid, blockUuid, fields, stor, parentType );
    }
#endif  // CONFIG_ADVANCED_SCENES
    else if( 0 == std::strcmp( methodName, method_names::Equal ) ) {
        std::tie( error, errorDescr ) = parseWhenBlockEqual( parentUuid, blockUuid, fields, stor, parentType );
    }
    else if( 0 == std::strcmp( methodName, method_names::IsUserLockOperation ) ) {
        std::tie( error, errorDescr ) =
            parseWhenBlockIsUserLockOperation( parentUuid, blockUuid, fields, stor, parentType );
    }
    else if( 0 == std::strcmp( methodName, method_names::HasAtLeastOneDictionaryValue ) ) {
        std::tie( error, errorDescr ) =
            parseWhenBlockHasAtLeastOneDictionaryValue( parentUuid, blockUuid, fields, stor, parentType );
    }
    else if( 0 == std::strcmp( methodName, method_names::IsDeviceDetected ) ) {
        constexpr auto isUnknowDeviceDetected = false;
        std::tie( error, errorDescr ) =
            parseWhenBlockIsDeviceDetected( parentUuid, blockUuid, fields, stor, parentType, isUnknowDeviceDetected );
    }
    else if( 0 == std::strcmp( methodName, method_names::IsUnknowDeviceDetected ) ) {
        constexpr auto isUnknowDeviceDetected = true;
        std::tie( error, errorDescr ) =
            parseWhenBlockIsDeviceDetected( parentUuid, blockUuid, fields, stor, parentType, isUnknowDeviceDetected );
    }
    else {
        return std::make_tuple( errors::ParseError, "parseWhenBlock: Unknow method name", Uuid() );
    }

    return std::make_tuple( error, std::move( errorDescr ), blockUuid );
}

std::tuple<int, std::string, UuidSet> parseWhenBlocksArray( const Uuid& sceneUuid,
                                                            const gason::JsonValue& jsonRoot,
                                                            storage::KeyValueStorage& stor,
                                                            const WhenBlock::ParentType parentType ) {
    if( !jsonRoot.isArray() ) {
        return std::make_tuple( errors::InvalidParams, "parseWhenBlocksArray: jsonRoot isn't json array", UuidSet() );
    }
    UuidSet uuidList;

    for( auto jsItr = gason::begin( jsonRoot ); jsItr.isValid(); ++jsItr ) {
        if( !jsItr->value.isObject() ) {
            return std::make_tuple(
                errors::InvalidParams, "parseWhenBlocksArray: Array item isn't json object!", UuidSet() );
        }

        int error;
        std::string errorDescr;
        Uuid blockUuid;
        std::tie( error, errorDescr, blockUuid ) = parseWhenBlock( sceneUuid, jsItr->value, stor, parentType );
        if( errors::InvalidParams == error ) {
            continue;
        }
        else if( errors::NotError != error ) {
            return std::make_tuple( error, std::move( errorDescr ), UuidSet() );
        }
        uuidList.emplace( blockUuid );
    }
    return std::make_tuple( errors::NotError, std::string(), std::move( uuidList ) );
}

DelayTypeSec parseDelay( gason::JsonValue& jsonRoot ) {
    auto delaySec = jsonRoot( tokens::Seconds ).toInt();
    delaySec += 60 * jsonRoot( tokens::Minutes ).toInt();
    delaySec += 3600 * jsonRoot( tokens::Hours ).toInt();
    delaySec += 24 * 3600 * jsonRoot( tokens::Days ).toInt();

    return DelayTypeSec( delaySec );
}

ActionsParser::ParseResult ActionsParser::parseSetItemValueThenBlock( gason::JsonValue& fields ) {
    int error = errors::NotError;
    std::string errorDescr;
    char* itemId = nullptr;
    common::IValue::UPtr itemValue;

    for( auto jsItr = gason::begin( fields ); jsItr.isValid() && error == errors::NotError; ++jsItr ) {
        StringType name, type;
        gason::JsonValue jsValue;
        std::tie( name, type, jsValue, error, errorDescr ) = parseNameTypeValue( jsItr->value );
        if( error != errors::NotError ) {
            return std::make_tuple( error, errorDescr, Uuid() );
        }

        if( name == tokens::Item ) {
            itemId = jsValue.toString();
        }
        else if( name == tokens::Value ) {
            itemValue = parseIValue( type, jsItr->value );
        }
    }

    if( !itemId ) {
        return std::make_tuple( errors::InvalidParams, "parseSetItemValueThenBlock: bad itemId!", Uuid() );
    }

    if( !itemValue ) {
        return std::make_tuple( errors::InvalidParams, "parseSetItemValueThenBlock: bad itemValue!", Uuid() );
    }

    auto itemInfo = gateways::ItemInfo::create( itemId );
    if( itemInfo ) {
        itemValue = itemInfo->castToItemType( std::move( itemValue ) );
        if( !itemValue ) {
            return std::make_tuple(
                errors::InvalidParams, "parseSetItemValueThenBlock: Wrong item value type!", Uuid() );
        }
    }

    if( !m_setVal ) {
        m_setVal = std::make_unique<ThenBlockSetValueMulticast3>( Uuid::generate(), m_sceneUuid, m_delay );
    }
    if( !m_setVal ) {
        return std::make_tuple( errors::InternalError,
                                "parseSetItemValueThenBlock: Can't create ThenBlockSetValueMulticast3 instance!",
                                Uuid() );
    }

    auto& valToItems = m_setVal->valueToItems();
    {
        ThenBlockSetValueMulticast3::KeyType key( itemValue->getId(), std::move( itemValue->clone() ) );
        auto& items = valToItems[std::move( key )];
        items.emplace_back( itemId );
    }

    {
        // previous key is not valide yet. Need create another one
        ThenBlockSetValueMulticast3::KeyType key( itemValue->getId(), std::move( itemValue->clone() ) );
        m_setVal->addToIndex( std::move( key ) );
    }
    return std::make_tuple( errors::NotError, std::string(), m_setVal->uuid() );
}

bool ActionsParser::trySaveThenBlockSetItemValue() {
    if( !m_setVal ) {
        return true;
    }

    auto itemsCount = m_setVal->itemsCount();
    if( 0 == itemsCount ) {
        return false;
    }

    if( 1 < itemsCount ) {
        bool isSuccess = m_setVal->save( m_stor );
        m_setVal.reset();
        return isSuccess;
    }

    auto itr = m_setVal->valueToItems().begin();
    ThenBlockSetItemValue thBlock( m_setVal->uuid(),
                                   m_sceneUuid,
                                   m_delay,
                                   std::move( *itr->second.begin() ),
                                   std::move( itr->first.second->clone() ) );
    bool isSuccess = thBlock.save( m_stor );
    m_setVal.reset();
    return isSuccess;
}

ActionsParser::ParseResult ActionsParser::parseSetDeviceArmedThenBlock( gason::JsonValue& fields ) {
    int error = errors::NotError;
    std::string errorDescr;
    std::string deviceId;
    bool isArmed = false;
    bool isArmedFieldFound = false;

    for( auto jsItr = gason::begin( fields ); jsItr.isValid(); ++jsItr ) {
        std::string name, type;
        gason::JsonValue jsValue;
        std::tie( name, type, jsValue, error, errorDescr ) = parseNameTypeValue( jsItr->value );
        if( error != errors::NotError ) {
            return std::make_tuple( error, errorDescr, Uuid() );
        }

        if( tokens::Device == name ) {
            if( tokens::Device == type ) {
                auto devIdCStr = jsValue.toString();
                if( !devIdCStr ) {
                    return std::make_tuple( errors::ParseError, errors::messages::ParseJsonFailed, Uuid() );
                }
                deviceId = devIdCStr;
            }
        }
        else if( name == tokens::DeviceFlag ) {
            if( type == scenes_types::Bool ) {
                isArmed = jsValue.toBool( &isArmedFieldFound );
                if( !isArmedFieldFound ) {
                    return std::make_tuple( errors::ParseError, errors::messages::ParseJsonFailed, Uuid() );
                }
            }
        }
    }

    if( deviceId.empty() || !isArmedFieldFound ) {
        return std::make_tuple( errors::ParseError, errors::messages::ParseJsonFailed, Uuid() );
    }

    auto uuid = Uuid::generate();
    ThenBlockSetDeviceArmed thBlock( uuid, m_sceneUuid, m_delay, std::move( deviceId ), isArmed );
    if( !thBlock.save( m_stor ) ) {
        return std::make_tuple( errors::InternalError,
                                "parseSetDeviceArmedThenBlock: Can't save ThenBlockSetDeviceArmed to storage!",
                                Uuid() );
    }

    return std::make_tuple( errors::NotError, std::string(), uuid );
}

ActionsParser::ParseResult ActionsParser::parseSendHttpRequestThenBlock( gason::JsonValue& fields ) {
    int error = errors::NotError;
    std::string errorDescr;
    std::string url;
    std::string user;
    std::string password;
    std::string request;
    std::string contentType;
    std::string content;
    std::map<std::string, std::string> headers;
    bool skipSecurity = false;

    for( auto jsItr = gason::begin( fields ); jsItr.isValid(); ++jsItr ) {
        std::string name, type;
        gason::JsonValue jsValue;
        std::tie( name, type, jsValue, error, errorDescr ) = parseNameTypeValue( jsItr->value );
        if( error != errors::NotError ) {
            return std::make_tuple( error, errorDescr, Uuid() );
        }

        if( name == tokens::Url && type == tokens::String ) {
            auto urlCStr = jsValue.toString();
            if( !urlCStr ) {
                return std::make_tuple( errors::ParseError, errors::messages::ParseJsonFailed, Uuid() );
            }
            url = urlCStr;
        }

        else if( name == tokens::Credential && tokens::Device == tokens::Credential ) {
            auto userCStr = jsValue( tokens::User ).toString();
            if( userCStr ) {
                user = userCStr;
            }

            auto passCStr = jsValue( tokens::Password ).toString();
            if( passCStr ) {
                password = passCStr;
            }
        }

        if( name == tokens::Request && type == tokens::String ) {
            auto reqCStr = jsValue.toString();
            if( reqCStr ) {
                request = reqCStr;
            }
        }

        if( name == tokens::ContentType && type == tokens::String ) {
            auto contTypeCStr = jsValue.toString();
            if( contTypeCStr ) {
                contentType = contTypeCStr;
            }
        }

        if( name == tokens::Content && type == tokens::String ) {
            auto contCStr = jsValue.toString();
            if( !contCStr ) {
                content = contCStr;
            }
        }

        if( name == "headers" && type == "dictionary" ) {
            auto dict = jsValue( tokens::Value );
            auto hedersDict = jsValue.toNode();
            while( hedersDict ) {
                if( strlen( hedersDict->key ) && hedersDict->value.isString() ) {
                    std::string key( hedersDict->key );
                    std::string value( hedersDict->value.toString() );
                    headers.insert( std::pair<std::string, std::string>( key, value ) );
                    hedersDict = hedersDict->next;
                }
            }
        }

        if( name == tokens::SkipSecurity && type == tokens::Bool ) {
            bool ok;
            bool val = jsValue.toBool( &ok );
            if( ok ) {
                skipSecurity = val;
            }
        }
    }

    auto uuid = Uuid::generate();

    ThenBlockSendHttpRequest thBlock( uuid,
                                      m_sceneUuid,
                                      m_delay,
                                      std::move( url ),
                                      std::move( request ),
                                      std::move( user ),
                                      std::move( password ),
                                      std::move( contentType ),
                                      std::move( content ),
                                      std::move( headers ),
                                      skipSecurity );
    if( !thBlock.save( m_stor ) ) {
        return std::make_tuple( errors::InternalError,
                                "parseSendHttpRequestThenBlock: Can't save ThenBlockSetDeviceArmed to storage!",
                                Uuid() );
    }

    return std::make_tuple( errors::NotError, std::string(), uuid );
}

#ifdef CONFIG_ADVANCED_SCENES
ActionsParser::ParseResult ActionsParser::parseSwitchHouseModeThenBlock( gason::JsonValue& fields ) {
    int error = errors::NotError;
    std::string errorDescr;
    std::string houseMode;

    for( auto jsItr = gason::begin( fields ); jsItr.isValid(); ++jsItr ) {
        std::string name, type;
        gason::JsonValue jsValue;
        std::tie( name, type, jsValue, error, errorDescr ) = parseNameTypeValue( jsItr->value );
        if( error != errors::NotError ) {
            return std::make_tuple( error, errorDescr, Uuid() );
        }

        if( tokens::HouseMode == name && tokens::HouseModeId == type ) {
            auto houseModeIdCStr = jsValue.toString();
            if( !houseModeIdCStr ) {
                return std::make_tuple( errors::ParseError, errors::messages::ParseJsonFailed, Uuid() );
            }
            houseMode = houseModeIdCStr;
        }
    }

    if( houseMode.empty() ) {
        return std::make_tuple( errors::ParseError, errors::messages::ParseJsonFailed, Uuid() );
    }

    auto uuid = Uuid::generate();
    ThenBlockSwitchHouseMode thBlock( uuid, m_sceneUuid, m_delay, std::move( houseMode ) );
    if( !thBlock.save( m_stor ) ) {
        return std::make_tuple( errors::InternalError,
                                "parseSwitchHouseModeThenBlock: Can't save ThenBlockSwitchHouseMode to storage!",
                                Uuid() );
    }

    return std::make_tuple( errors::NotError, std::string(), uuid );
}

ActionsParser::ParseResult ActionsParser::parseRunSceneThenBlock( gason::JsonValue& fields ) {
    int error = errors::NotError;
    std::string errorDescr;
    StringType sceneStorId;

    for( auto jsItr = gason::begin( fields ); jsItr.isValid(); ++jsItr ) {
        std::string name, type;
        gason::JsonValue jsValue;
        std::tie( name, type, jsValue, error, errorDescr ) = parseNameTypeValue( jsItr->value );
        if( error != errors::NotError ) {
            return std::make_tuple( error, errorDescr, Uuid() );
        }

        if( tokens::Scene == name && tokens::SceneId == type ) {
            auto sceneStorIdCStr = jsValue.toString();
            if( !sceneStorIdCStr ) {
                return std::make_tuple( errors::ParseError, errors::messages::ParseJsonFailed, Uuid() );
            }
            sceneStorId = sceneStorIdCStr;
        }
    }

    if( sceneStorId.empty() ) {
        return std::make_tuple( errors::ParseError, errors::messages::ParseJsonFailed, Uuid() );
    }

    auto uuid = Uuid::generate();
    ThenBlockExecuteScene thBlock( uuid, m_sceneUuid, m_delay, std::move( sceneStorId ) );
    if( !thBlock.save( m_stor ) ) {
        return std::make_tuple(
            errors::InternalError, "parseRunSceneThenBlock: Can't save ThenBlockExecuteScene to storage!", Uuid() );
    }

    return std::make_tuple( errors::NotError, std::string(), uuid );
}
#endif  // CONFIG_ADVANCED_SCENES

ActionsParser::ParseResult ActionsParser::parseCloudCommandThenBlock( gason::JsonValue& fields ) {
    int error = errors::NotError;
    std::string errorDescr;
    StringType commandId;
    StringType capability;
    StringType command;
    ValueTypeUPtr value;

    for( auto jsItr = gason::begin( fields ); jsItr.isValid(); ++jsItr ) {
        std::string name, type;
        gason::JsonValue jsValue;
        std::tie( name, type, jsValue, error, errorDescr ) = parseNameTypeValue( jsItr->value );
        if( error != errors::NotError ) {
            return std::make_tuple( error, errorDescr, Uuid() );
        }

        if( tokens::Abstract == name && scenes_types::String == type ) {
            auto cStr = jsValue.toString();
            if( !cStr ) {
                break;
            }
            commandId = cStr;
        }
        else if( tokens::Capability == name && scenes_types::String == type ) {
            auto cStr = jsValue.toString();
            if( !cStr ) {
                break;
            }
            capability = cStr;
        }
        else if( tokens::Command == name && scenes_types::String == type ) {
            auto cStr = jsValue.toString();
            if( !cStr ) {
                break;
            }
            command = cStr;
        }
        else if( tokens::Parameters == name && tokens::Object == type ) {
            value = parseIValue( type, jsItr->value );
        }
    }

    if( commandId.empty() || capability.empty() || command.empty() || !value ) {
        return std::make_tuple( errors::ParseError, errors::messages::ParseJsonFailed, Uuid() );
    }

    auto uuid = Uuid::generate();
    ThenBlockCloudCommand thBlock( uuid,
                                   m_sceneUuid,
                                   m_delay,
                                   std::move( commandId ),
                                   std::move( capability ),
                                   std::move( command ),
                                   std::move( value ) );
    if( !thBlock.save( m_stor ) ) {
        return std::make_tuple(
            errors::InternalError, "parseThenBlockCloudCommand: Can't save ThenBlockCloudCommand to storage!", Uuid() );
    }

    return std::make_tuple( errors::NotError, std::string(), uuid );
}

ActionsParser::ParseResult ActionsParser::parseThenBlock( gason::JsonValue& jsonRoot ) {
    if( !jsonRoot.isObject() ) {
        return std::make_tuple( errors::InvalidParams, "parseThenBlock: jsonRoot isn't json object", Uuid() );
    }

    const auto methodName = jsonRoot( tokens::BlockOptions )( tokens::Method )( tokens::Name ).toString();
    if( !methodName ) {
        return std::make_tuple( errors::InvalidParams, "parseThenBlock: method name isn't exist", Uuid() );
    }

    int error;
    std::string errorDescr;
    Uuid uuid;

    gason::JsonValue fields = jsonRoot( tokens::Fields );
    if( !fields.isArray() ) {
        return std::make_tuple( errors::InvalidParams, "parseThenBlock: \"fields\" isn't json array!", Uuid() );
    }

    {
        auto delayJs = jsonRoot( tokens::Delay );
        const auto newDelay = parseDelay( delayJs );
        if( 0 < newDelay.count() && !trySaveThenBlockSetItemValue() ) {
            return std::make_tuple(
                errors::InternalError, "parseThenBlock: Can't save ThenBlockSetValueMulticast2 to storage!", Uuid() );
        }
        m_delay = newDelay;
    }

    if( 0 == std::strcmp( methodName, modifiers::SetItemValue ) ) {
        std::tie( error, errorDescr, uuid ) = parseSetItemValueThenBlock( fields );
    }
    else if( 0 == std::strcmp( methodName, modifiers::SetDeviceArmed ) ) {
        std::tie( error, errorDescr, uuid ) = parseSetDeviceArmedThenBlock( fields );
    }

    else if( 0 == std::strcmp( methodName, modifiers::sendHttpRequest ) ) {
        std::tie( error, errorDescr, uuid ) = parseSendHttpRequestThenBlock( fields );
    }
#ifdef CONFIG_ADVANCED_SCENES
    else if( 0 == std::strcmp( methodName, modifiers::SwitchHouseMode ) ) {
        std::tie( error, errorDescr, uuid ) = parseSwitchHouseModeThenBlock( fields );
    }
    else if( 0 == std::strcmp( methodName, modifiers::RunScene ) ) {
        std::tie( error, errorDescr, uuid ) = parseRunSceneThenBlock( fields );
    }
#endif  // CONFIG_ADVANCED_SCENES
    else if( 0 == std::strcmp( methodName, modifiers::CloudCommand ) ) {
        std::tie( error, errorDescr, uuid ) = parseCloudCommandThenBlock( fields );
    }
    else {
        std::make_tuple( errors::ApplicationError, "parseThenBlock: Unknow method name \"%s\"", methodName );
    }

    return std::make_tuple( error, std::move( errorDescr ), uuid );
}

std::tuple<int, std::string, UuidList> ActionsParser::parse( gason::JsonValue& jsonRoot ) {
    if( !jsonRoot.isArray() ) {
        return std::make_tuple( errors::InvalidParams, "parseThenBlocksArray: jsonRoot isn't json array", UuidList() );
    }
    UuidList uuidList;
    for( auto jsItr = gason::begin( jsonRoot ); jsItr.isValid(); ++jsItr ) {
        if( !jsItr->value.isObject() ) {
            return std::make_tuple(
                errors::InvalidParams, "parseThenBlocksArray: Array item isn't json object!", UuidList() );
        }

        int error;
        std::string errorDescr;
        Uuid blockUuid;
        std::tie( error, errorDescr, blockUuid ) = parseThenBlock( jsItr->value );
        if( errors::NotError != error ) {
            return std::make_tuple( error, std::move( errorDescr ), UuidList() );
        }

        if( !uuidList.empty() ) {
            auto itr = --uuidList.end();
            if( *itr == blockUuid ) {
                continue;
            }
        }

        uuidList.emplace_back( blockUuid );
    }
    if( !trySaveThenBlockSetItemValue() ) {
        return std::make_tuple( errors::InternalError,
                                "parseSetItemValueThenBlock: Can't save ThenBlockSetValueMulticast3 to storage!",
                                UuidList() );
    }

    return std::make_tuple( errors::NotError, std::string(), std::move( uuidList ) );
}

std::tuple<int, std::string, LocalScene::HouseModesList> parseHouseModes( gason::JsonValue& jsonRoot ) {
    if( !jsonRoot ) {
        return std::make_tuple( errors::NotError, std::string(), LocalScene::HouseModesList( { "1", "2", "3", "4" } ) );
    }

    if( !jsonRoot.isArray() ) {
        return std::make_tuple(
            errors::InvalidParams, "parseHouseModes: jsonRoot isn't json array", LocalScene::HouseModesList() );
    }

    LocalScene::HouseModesList houseModes;
    auto jsItr = gason::begin( jsonRoot );
    while( jsItr.isValid() ) {
        char* modeStr = jsItr->value.toString();
        if( !modeStr ) {
            return std::make_tuple(
                errors::InvalidParams, "parseHouseModes: Item isn't json string!", LocalScene::HouseModesList() );
        }

        houseModes.emplace_back( modeStr );

        ++jsItr;
    }

    return std::make_tuple( errors::NotError, std::string(), std::move( houseModes ) );
}

std::tuple<int, std::string, LocalScene::UserNotificationsList> parseUserNotifications( gason::JsonValue& jsonRoot ) {
    if( !jsonRoot.isArray() ) {
        return std::make_tuple( errors::InvalidParams,
                                "parseUserNotifications: jsonRoot isn't json array",
                                LocalScene::UserNotificationsList() );
    }
    LocalScene::UserNotificationsList notifications;
    auto jsItr = gason::begin( jsonRoot );
    while( jsItr.isValid() ) {
        char* notifStr = jsItr->value.toString();
        if( !notifStr ) {
            return std::make_tuple( errors::InvalidParams,
                                    "parseUserNotifications: Item isn't json string!",
                                    LocalScene::UserNotificationsList() );
        }

        notifications.emplace_back( notifStr );

        ++jsItr;
    }

    return std::make_tuple( errors::NotError, std::string(), std::move( notifications ) );
}

template <>
std::tuple<int, std::string, LocalScene::UPtr> parseScene<LocalScene>( gason::JsonValue& jsonRoot,
                                                                       const Uuid& sceneUuid,
                                                                       storage::KeyValueStorage& stor ) {
    using SceneType = LocalScene;

    if( !jsonRoot.isObject() ) {
        return std::make_tuple( errors::InvalidParams, "parseScene: jsonRoot isn't json object", nullptr );
    }

    StringType name;
    {
        auto cStrName = jsonRoot( tokens::Name ).toString();
        if( cStrName ) {
            name = cStrName;
        }
    }

    const auto sRoomId = jsonRoot( tokens::ParentId ).toString();
    const auto sGroupId = jsonRoot( tokens::GroupId ).toString();

    SceneType::ExecPolicy execPolicy = SceneType::ExecPolicy::IgnoreActionResult;
    auto policyCStr = jsonRoot( tokens::ExecPolicyField ).toString();
    if( policyCStr ) {
        if( 0 ==
            std::strncmp( policyCStr, tokens::ExecPolicyCheckResult, std::strlen( tokens::ExecPolicyCheckResult ) ) ) {
            execPolicy = SceneType::ExecPolicy::CheckActionResult;
        }
        else if( 0 != std::strncmp( policyCStr,
                                    tokens::ExecPolicyIgnoreResult,
                                    std::strlen( tokens::ExecPolicyIgnoreResult ) ) ) {
            FUNC_LOGW( "Wrong \"exec_policy\" value: %s!", policyCStr );
        }
    }

    auto whenBlockJson = jsonRoot( tokens::When );
    int error;
    std::string errorDescr;
    UuidSet whenUuidList;
    std::tie( error, errorDescr, whenUuidList ) =
        parseWhenBlocksArray( sceneUuid, whenBlockJson, stor, WhenBlock::ParentType::Scene );
    if( errors::NotError != error ) {
        return std::make_tuple( error, std::move( errorDescr ), nullptr );
    }

    auto thenBlockJson = jsonRoot( tokens::Then );
    ActionsParser actionParser( sceneUuid, stor );
    UuidList thenUuidList;
    std::tie( error, errorDescr, thenUuidList ) = actionParser.parse( thenBlockJson );
    if( errors::NotError != error ) {
        return std::make_tuple( error, std::move( errorDescr ), nullptr );
    }

    SceneType::HouseModesList houseModes;
    auto sHouseModes = jsonRoot( tokens::ScenesHouseModes );
    if( sHouseModes ) {
        std::tie( error, errorDescr, houseModes ) = parseHouseModes( sHouseModes );
        if( errors::NotError != error ) {
            return std::make_tuple( error, std::move( errorDescr ), nullptr );
        }
    }

    SceneType::UserNotificationsList userNotifications;
    auto sUserNotifications = jsonRoot( tokens::UserNotifications );
    if( sUserNotifications ) {
        std::tie( error, errorDescr, userNotifications ) = parseUserNotifications( sUserNotifications );
        if( errors::NotError != error ) {
            return std::make_tuple( error, std::move( errorDescr ), nullptr );
        }
    }

    bool isSuccess = false;
    bool isEnabled = jsonRoot( tokens::Enabled ).toBool( &isSuccess );
    if( !isSuccess ) {
        return std::make_tuple( errors::ParseError, "parseScene: Can't parse \"enabled\" token!", nullptr );
    }

    auto scene = std::make_unique<SceneType>( sceneUuid,
                                              std::move( name ),
                                              sRoomId ? sRoomId : "",
                                              sGroupId ? sGroupId : "",
                                              std::move( whenUuidList ),
                                              std::move( thenUuidList ),
                                              std::move( houseModes ),
                                              std::move( userNotifications ),
                                              isEnabled,
                                              execPolicy );
    if( !scene ) {
        return std::make_tuple( errors::ApplicationError, "Can't create scene in heap!", nullptr );
    }

    auto sceneId = jsonRoot( tokens::SubId ).toString();
    if( sceneId ) {
        scene->setPersistentStorId( sceneId );
    }

    return std::make_tuple( errors::NotError, std::string(), std::move( scene ) );
}

std::tuple<int, std::string> saveSceneToDbAndFile( const StringType& scenesPath,
                                                   gason::JsonValue& jsonRoot,
                                                   const Uuid& sceneUuid,
                                                   storKeyClbk storKeyClbk ) {
    if( !jsonRoot.isObject() ) {
        return std::make_tuple( errors::InvalidParams, "saveScene: jsonRoot isn't json object" );
    }

    auto fName = scenesPath + "/" + storKeyClbk( sceneUuid );

    auto fStream = storage::WriteFileStream::open( fName.c_str(), "w+" );
    if( !fStream ) {
        return std::make_tuple( errors::InvalidParams, "saveScene: Bad stream" );
    }

    common::JsonStreamGenerator jsGen( *fStream );
    jsGen.setFirstItemState();
    jsGen.setDept( 1 );

    auto end = gason::end( jsonRoot );
    for( auto itr = gason::begin( jsonRoot ); itr != end; ++itr ) {
        StringType key( itr->key );
        if( key == tokens::Enabled || key == tokens::SubId || key == tokens::IsGroup || key == tokens::GroupId ||
            key == tokens::ParentId || key == tokens::UserNotifications || key == tokens::ScenesHouseModes ||
            key == tokens::Name || key == tokens::Tag ) {
            continue;
        }

        if( !jsGen.writeProperty( itr->key, itr->value ) ) {
            return std::make_tuple( errors::InvalidParams, "!jsGen.writeProperty" );
        }
    }

    jsGen.setDept( 0 );

    return std::make_tuple( errors::NotError, std::string() );
}

}  // end of namespace scenes
