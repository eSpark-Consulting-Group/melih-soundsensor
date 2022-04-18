
#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "DebugUtils.h"
#include <GatewayUtils.h>
#include <DeviceInfo.h>
#include <PagesManager.h>
#include <CommonResources.h>
#include <string.h>
#include <tuple>
#include "scenes/commands/ScenesList.h"
#include "scenes/commands/ScenesData.h"
#include "scenes/commands/JsonifyScene.h"
#include "FeatureFlagController.h"

#include "HubSettings.h"

namespace nma {

using namespace common::resources;
using namespace core;
namespace {

constexpr char Ids[] = "ids";
constexpr char All[] = "*";

//  See docs: https://log.ezlo.com/new/hub/common/#hubdatalist
struct HubDataItemInfo {
    std::vector<std::string> ids;                  // #TODO We should use std::set<std::string>
    std::vector<std::vector<std::string>> fields;  // #TODO We should use std::map<std::string, std::set<std::string>>
};
static bool getHubDataItemInfo( gason::JsonValue params,
                                std::string property,
                                HubDataItemInfo& info,
                                const std::string& indexes = Ids ) {
    const auto items = params( property.c_str() );

    if( params( property.c_str() ) && !items.isObject() )
        return false;

    const auto itemsIds = items( indexes.c_str() );

    if( itemsIds ) {
        if( itemsIds.isArray() ) {
            for( auto itemsId : itemsIds ) {
                if( itemsId.value.isString() ) {
                    auto str = itemsId.value.toString();
                    if( str ) {
                        info.ids.push_back( str );
                    }
                }
                else {
                    return false;
                }
            }
        }
    }

    if( info.ids.empty() ) {
        info.ids.push_back( All );
    }

    const auto itemsFields = items( tokens::Fields );
    const auto devicesFieldsInc = itemsFields( tokens::Include );
    std::vector<std::string> incs;

    if( devicesFieldsInc.isArray() ) {
        for( auto inc : devicesFieldsInc ) {
            auto str = inc.value.toString();
            if( str ) {
                std::string value( str );
                incs.push_back( value );
            }
        }
        if( !incs.empty() )
            info.fields.push_back( incs );
    }

    return true;
}

static bool createRoomsAnswer( HubDataItemInfo filter, common::JsonStreamGenerator& jsonGenerator ) {
    auto pagesManager = core::PagesManager::get();
    if( !pagesManager ) {
        return false;
    }
    bool result = jsonGenerator.writePropertyKey( tokens::Rooms );
    result = result && jsonGenerator.writeOpeningArray();
    result = result && filter.ids.front() == All ? pagesManager->listPages( jsonGenerator ) :
                                                   pagesManager->listPagesWithFilter( jsonGenerator, filter.ids );

    result = result && jsonGenerator.writeClosingSymbol();
    return result;
}

static bool createDevicesAnswer( HubDataItemInfo filter, common::JsonStreamGenerator& jsonGenerator ) {
    bool result = jsonGenerator.writePropertyKey( tokens::Devices );
    result = result && jsonGenerator.writeOpeningObject();

    for( auto deviceId : filter.ids ) {
        gateways::enumerateDevicesFromDb(
            [deviceId, filter, &result, &jsonGenerator]( const gateways::BaseDeviceInfo& deviceInfo ) {
                if( deviceInfo.getId() == deviceId || deviceId == All ) {
                    result = result && jsonGenerator.writePropertyKey( deviceInfo.getId() );
                    result = result && jsonGenerator.writeOpeningObject();
                    if( filter.fields.empty() ) {
                        result = gateways::utils::writeDeviceInfo( jsonGenerator, deviceInfo );
                    }
                    else {
                        for( auto field : filter.fields ) {
                            result = gateways::utils::writeDeviceInfoWithFilter( jsonGenerator, deviceInfo, field );
                        }
                    }
                    result = result && jsonGenerator.writeClosingSymbol();
                }
                return true;
            } );
    }

    result = result && jsonGenerator.writeClosingSymbol();
    return result;
}

static bool createItemsAnswer( HubDataItemInfo filter, common::JsonStreamGenerator& jsonGenerator ) {
    bool result = jsonGenerator.writePropertyKey( tokens::Items );
    result = result && jsonGenerator.writeOpeningObject();

    for( auto itemsId : filter.ids ) {
        gateways::enumerateItemsFromDb(
            [itemsId, filter, &result, &jsonGenerator]( const gateways::ItemInfo& itemInfo ) {
                if( itemsId == All || itemInfo.getId() == itemsId ) {
                    result = result && jsonGenerator.writePropertyKey( itemInfo.getId().c_str() );
                    result = result && jsonGenerator.writeOpeningObject();

                    if( filter.fields.empty() ) {
                        result = gateways::utils::writeItemInfo( jsonGenerator, itemInfo );
                    }
                    else {
                        for( auto field : filter.fields ) {
                            result = gateways::utils::writeItemInfoWithFilter( jsonGenerator, itemInfo, field );
                        }
                    }
                    result = result && jsonGenerator.writeClosingSymbol();
                }

                return true;
            } );
    }

    result = result && jsonGenerator.writeClosingSymbol();

    return result;
}

WebSocketHandler::TaskClbkType createScenesAnswer( HubDataItemInfo filter,
                                                   common::JsonStreamGenerator& jsGen,
                                                   NmaBase& client ) {
    using ResponceType = WebSocketHandler::ResponceType;
    using StringType = WebSocketHandler::StringType;

    if( !jsGen.writePropertyKey( tokens::Scenes ) ) {
        return nullptr;
    }

    std::set<std::string> scenesId;
    if( !filter.ids.empty() && filter.ids.front() != All ) {
        scenesId.insert( filter.ids.begin(), filter.ids.end() );
    }

    std::set<std::string> scenesFields;
    for( const auto& vec : filter.fields ) {
        scenesFields.insert( vec.begin(), vec.end() );
    }

    auto resp = client.asyncSend<scenes::commands::ScenesData>(
        scenes::commands::LOCAL_BUS_NAME, jsGen, std::move( scenesId ), std::move( scenesFields ) );
    auto asyncResp = std::shared_future( std::move( resp ) );

    auto clbk = [asyncResp]( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) -> ResponceType {
        if( !asyncResp.valid() ) {
            return std::make_tuple( errors::Waiting, StringType() );
        }

        auto [errorCode, errorDescr] = asyncResp.get();
        if( errors::NotError != errorCode ) {
            return std::make_tuple( errorCode, std::move( errorDescr ) );
        }

        if( !gen.writeClosingSymbol() ) {
            return std::make_tuple( errors::GenJsonFailed, StringType{} );
        }

        return std::make_tuple( errors::NotError, StringType{} );
    };
    return std::move( clbk );
}

static bool createSettingsAnswer( HubDataItemInfo filter,
                                  common::JsonStreamGenerator& jsonGenerator,
                                  NmaBase& client ) {
    bool result = jsonGenerator.writePropertyKey( tokens::Settings );
    result = result && jsonGenerator.writeOpeningObject();

    std::vector<std::string> enumVal;
    std::string value;
    std::string type;
    std::map<std::string, int> dict;

    auto createSettingsTokenJson =
        [&enumVal, &value, &type, &result, &jsonGenerator]( std::vector<std::string> fields ) {
            bool allFields = fields.front() == All;

            for( std::string& field : fields ) {
                if( field == tokens::Value || allFields ) {
                    result = result && jsonGenerator.writeProperty( tokens::Value, value.c_str() );
                }
                if( field == tokens::Type || allFields ) {
                    result = result && jsonGenerator.writeProperty( tokens::Type, type );
                }
                if( type == tokens::Enum && ( field == tokens::Enum || allFields ) ) {
                    result = result && jsonGenerator.writePropertyKey( tokens::Enum );
                    result = result && jsonGenerator.writeOpeningArray();
                    for( auto val : enumVal ) {
                        result = result && jsonGenerator.writeProperty( nullptr, val.c_str() );
                    }
                    result = result && jsonGenerator.writeClosingSymbol();
                }
            }

            return result;
        };

    auto createSettingsDictionaryJson =
        [&enumVal, &dict, &type, &result, &jsonGenerator]( std::vector<std::string> fields ) {
            bool allFields = fields.front() == All;

            for( std::string& field : fields ) {
                for( const auto& item : dict ) {
                    if( item.first == field || allFields )
                        result = result && jsonGenerator.writeProperty( item.first.c_str(), item.second );
                }
            }

            return result;
        };

    auto createSettingsBoolJson = [&value, &type, &result, &jsonGenerator]( std::vector<std::string> fields,
                                                                            bool value ) {
        bool allFields = fields.front() == All;
        for( std::string& field : fields ) {
            if( field == tokens::Value || allFields ) {
                result = result && jsonGenerator.writeProperty( tokens::Value, value );
            }
            if( field == tokens::Type || allFields ) {
                result = result && jsonGenerator.writeProperty( tokens::Type, type );
            }
            if( field == tokens::Enum || allFields ) {
                result = result && jsonGenerator.writePropertyKey( tokens::Enum );
                result = result && jsonGenerator.writeOpeningArray();
                result = result && jsonGenerator.writeProperty( nullptr, true );
                result = result && jsonGenerator.writeProperty( nullptr, false );
                result = result && jsonGenerator.writeClosingSymbol();
            }
        }

        return result;
    };

    auto applyFilter = [&result, &createSettingsTokenJson]( std::vector<std::vector<std::string>> fields ) {
        if( fields.empty() ) {
            result = result && createSettingsTokenJson( { All } );
        }
        else {
            for( auto field : fields ) {
                result = result && createSettingsTokenJson( field );
            }
        }
    };

    auto applyDictionaryFilter = [&result,
                                  &createSettingsDictionaryJson]( std::vector<std::vector<std::string>> fields ) {
        if( fields.empty() ) {
            result = result && createSettingsDictionaryJson( { All } );
        }
        else {
            for( auto field : fields ) {
                result = result && createSettingsDictionaryJson( field );
            }
        }
    };

    for( auto name : filter.ids ) {
        if( name == tokens::ScaleTemperature || name == All ) {
            TemperatureScaleEnum val;
            enumVal = HubSettingsInfo::temperatureScaleValues();
            HubSettings::get().getTemperatureScale( val, type );
            value = HubSettingsInfo::temperatureScaleToStr( val );
            result = result && jsonGenerator.writePropertyKey( tokens::ScaleTemperature );
            result = result && jsonGenerator.writeOpeningObject();

            applyFilter( filter.fields );
            result = result && jsonGenerator.writeClosingSymbol();
        }
        if( name == tokens::TimeFormat || name == All ) {
            TimeFormatEnum val;
            enumVal = HubSettingsInfo::timeFormatValues();
            HubSettings::get().getTimeFormat( val, type );
            value = HubSettingsInfo::timeFormatToStr( val );
            result = result && jsonGenerator.writePropertyKey( tokens::TimeFormat );
            result = result && jsonGenerator.writeOpeningObject();
            applyFilter( filter.fields );
            result = result && jsonGenerator.writeClosingSymbol();
        }
        if( name == tokens::DateFormat || name == All ) {
            DateFormatEnum val;
            enumVal = HubSettingsInfo::dateFormatValues();
            HubSettings::get().getDateFormat( val, type );
            value = HubSettingsInfo::dateFormatToStr( val );
            result = result && jsonGenerator.writePropertyKey( tokens::DateFormat );
            result = result && jsonGenerator.writeOpeningObject();
            applyFilter( filter.fields );
            result = result && jsonGenerator.writeClosingSymbol();
        }

        if( name == tokens::LocationUUID || name == All ) {
            value = HubSettings::get().getLocationUuid();
            type = HubSettings::get().getLocationUuidType();
            result = result && jsonGenerator.writePropertyKey( tokens::LocationUUID );
            result = result && jsonGenerator.writeOpeningObject();
            applyFilter( filter.fields );
            result = result && jsonGenerator.writeClosingSymbol();
        }

        if( name == tokens::BroadcastsDelay || name == All ) {
            dict = HubSettings::get().getBroadcastDelays();
            type = tokens::Dictionary;
            result = result && jsonGenerator.writePropertyKey( tokens::BroadcastsDelay );
            result = result && jsonGenerator.writeOpeningObject();
            applyDictionaryFilter( filter.fields );
            result = result && jsonGenerator.writeClosingSymbol();
        }

        if( name == tokens::FirstStart || name == All ) {
            bool val = false;
            HubSettings::get().getFirstStart( val, type );
            result = result && jsonGenerator.writePropertyKey( tokens::FirstStart );
            result = result && jsonGenerator.writeOpeningObject();

            if( filter.fields.empty() ) {
                result = result && createSettingsBoolJson( { All }, val );
            }
            else {
                for( auto field : filter.fields ) {
                    result = result && createSettingsBoolJson( field, val );
                }
            }
            result = result && jsonGenerator.writeClosingSymbol();
        }
    }

    return result && jsonGenerator.writeClosingSymbol();
}
}  // namespace

WebSocketHandler::ResponceType WebSocketHandler::onHubDataListRequest( const gason::JsonValue& json,
                                                                       common::JsonStreamGenerator& jsonGenerator ) {
    auto params = json( tokens::Params );
    if( !params || !params.isObject() ) {
        FUNC_LOGE( "params field is missing or not json object" );
        return std::make_tuple( errors::ParseError, "\"params\" field is missing or not json object" );
    }

    bool result = false;

    bool checkParams = false;

    result = jsonGenerator.writeOpeningObject();

    if( params.hasKey( tokens::Devices ) ) {
        checkParams = true;
        HubDataItemInfo devicesInfo;
        if( getHubDataItemInfo( params, tokens::Devices, devicesInfo ) ) {
            result = createDevicesAnswer( devicesInfo, jsonGenerator );
        }
        else {
            jsonGenerator.writeClosingSymbol();
            return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
        }
    }

    if( params.hasKey( tokens::Items ) ) {
        checkParams = true;
        HubDataItemInfo itemsInfo;
        if( getHubDataItemInfo( params, tokens::Items, itemsInfo ) ) {
            result = createItemsAnswer( itemsInfo, jsonGenerator );
        }
        else {
            jsonGenerator.writeClosingSymbol();
            return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
        }
    }

    if( params.hasKey( tokens::Rooms ) ) {
        checkParams = true;
        HubDataItemInfo roomsInfo;
        if( getHubDataItemInfo( params, tokens::Rooms, roomsInfo ) ) {
            createRoomsAnswer( roomsInfo, jsonGenerator );
        }
        else {
            jsonGenerator.writeClosingSymbol();
            return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
        }
    }

    if( params.hasKey( tokens::Settings ) ) {
        checkParams = true;
        HubDataItemInfo settingsInfo;
        if( getHubDataItemInfo( params, tokens::Settings, settingsInfo, tokens::Names ) ) {
            createSettingsAnswer( settingsInfo, jsonGenerator, m_client );
        }
        else {
            jsonGenerator.writeClosingSymbol();
            return std::make_tuple( errors::InternalError, errors::data::WrongParams );
        }
    }

    if( params.hasKey( tokens::Features ) ) {
        checkParams = true;
        auto featureFlagController = core::features::FeatureFlagController::get();
        if( !featureFlagController ) {
            jsonGenerator.writeEmptyObject() && jsonGenerator.writeItemDelimiter();
            return std::make_tuple( errors::ApplicationError, errors::messages::FeatureFlagControllerNotSet );
        }
        std::vector<features::FeatureFlag> features;
        auto requiredNames = params( tokens::Features )( tokens::Names );
        if( requiredNames ) {
            if( !requiredNames.isArray() ) {
                jsonGenerator.writeOpeningObject() && jsonGenerator.writeClosingSymbol();
                return std::make_tuple( errors::InvalidParams, "names value isn't array!" );
            }
            else {
                int namesCount = 0;
                for( const auto& name : requiredNames ) {
                    namesCount++;
                    if( features::FeatureFlag::typeFromStr( name.value.toString() ).first == true ) {
                        features.push_back( featureFlagController->getFeatureFlag(
                            features::FeatureFlag::typeFromStr( name.value.toString() ).second ) );
                    }
                }

                if( namesCount == 0 ) {
                    features = featureFlagController->getFeatureFlagList();
                }
            }
        }
        else {
            features = featureFlagController->getFeatureFlagList();
        }

        std::vector<std::string> fields;
        auto requiredFields = params( tokens::Features )( tokens::Fields );
        if( requiredFields ) {
            auto includes = params( tokens::Features )( tokens::Fields )( tokens::Include );
            if( includes ) {
                for( const auto& incl : includes ) {
                    fields.push_back( incl.value.toString() );
                }
            }
        }
        jsonGenerator.writePropertyKey( tokens::Features );
        jsonGenerator.writeOpeningObject();
        for( const auto& feature : features ) {
            jsonGenerator.writePropertyKey( feature.typeStr() );
            jsonGenerator.writeOpeningObject();
            if( fields.empty() ) {
                jsonGenerator.writeProperty( tokens::Status, feature.statusStr() );
                jsonGenerator.writeProperty( tokens::Version, feature.version() );
            }
            else {
                for( const auto& field : fields ) {
                    if( field == "version" ) {
                        jsonGenerator.writeProperty( tokens::Version, feature.version() );
                    }
                    else if( field == "status" ) {
                        jsonGenerator.writeProperty( tokens::Status, feature.statusStr() );
                    }
                }
            }
            jsonGenerator.writeClosingSymbol();
        }

        jsonGenerator.writeClosingSymbol();
    }

    if( params.hasKey( tokens::Scenes ) ) {
        checkParams = true;
        HubDataItemInfo scenesInfo;
        if( getHubDataItemInfo( params, tokens::Scenes, scenesInfo ) ) {
            m_taskClbk = createScenesAnswer( scenesInfo, jsonGenerator, m_client );
            if( m_taskClbk ) {
                return std::make_tuple( errors::Deffered, StringType{} );
            }
            result = false;
        }
        else {
            jsonGenerator.writeClosingSymbol();
            return std::make_tuple( errors::InternalError, errors::data::WrongParams );
        }
    }

    result = result && jsonGenerator.writeClosingSymbol();

    return result && checkParams ? std::make_tuple( errors::NotError, StringType{} ) :
                                   std::make_tuple( errors::InternalError, errors::data::WrongParams );
}

}  // namespace nma
