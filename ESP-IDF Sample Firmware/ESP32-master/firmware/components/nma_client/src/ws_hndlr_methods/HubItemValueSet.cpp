#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "BinaryFileStorageFactory.h"
#include "StorageSection.h"
#include "DeviceInfo.h"
#include "ObjectSerializer.h"
#include "BusCommandCloudItemValueSet.h"
#include "IValueCast.h"
#include <tuple>

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubItemValueSet( const gason::JsonValue& json,
                                                                    common::JsonStreamGenerator& gen ) {
    gen.writeEmptyObject() && gen.writeItemDelimiter();

    auto params = json( tokens::Params );

    using ItemsType = decltype( bus::command::CloudItemValueSet::items );
    ItemsType items;
    auto jsItemId = params( tokens::SubIdMulti );
    if( jsItemId.isArray() ) {
        for( const auto item : jsItemId ) {
            const char* cstr = item.value.toString();
            if( cstr ) {
                items.emplace_back( cstr );
            }
        }
    }
    else {
        auto ItemId = params( tokens::SubId );
        if( ItemId.isString() ) {
            items.emplace_back( ItemId.toString() );
        }
        if( items.empty() ) {
            return std::make_tuple( errors::InvalidParams, errors::data::EmptyIdField );
        }
        bool ready = gateways::BaseDeviceInfo::isReadyByItemId( items.front().c_str() );
        if( !ready ) {
            return std::make_tuple( errors::InternalError, errors::messages::DeviceBusy );
        }
    }

    if( items.empty() ) {
        return std::make_tuple( errors::InvalidRequest, errors::data::EmptyIdField );
    }

    if( !params( tokens::Value ) ) {
        return std::make_tuple( errors::InvalidRequest, errors::data::EmptyIdField );
    }

    auto value = common::makeIval( params );
    if( !value ) {
        return std::make_tuple( errors::InvalidParams, errors::data::WrongType );
    }

    std::map<common::IValue::UPtr, ItemsType> valueToItems;

    for( const auto itemId : items ) {
        auto info = gateways::ItemInfo::create( itemId.c_str() );
        if( !info ) {
            return std::make_tuple( errors::ApplicationError, errors::data::ObjectNotFound );
        }

        auto newVal = info->castToItemType( value->clone() );
        if( !newVal ) {
            return std::make_tuple( errors::InvalidParams, errors::data::WrongType );
        }

        decltype( valueToItems )::iterator itr;
        std::tie( itr, std::ignore ) = valueToItems.emplace( std::move( newVal ), ItemsType() );

        if( !info->hasSetter() ) {
            return std::make_tuple( errors::InvalidRequest, errors::data::UnexpectedRequest );
        }

        if( info->isMinMax() ) {
            auto fval = common::iValueCast<float>( value );
            if( std::isnan( fval ) ) {
                return std::make_tuple( errors::InvalidParams, errors::data::WrongType );
            }

            const auto minmax = info->getMinMax();
            // TODO: Workaround until a new item is added to set the value for  dimmer to 255
            if( !( info->getName() == common::resources::items::Dimmer &&
                   fval == common::resources::items::values::DimmerFavorite ) ) {
                if( minmax.first > fval || minmax.second < fval ) {
                    return std::make_tuple( errors::InvalidParams, errors::data::RangeInvalid );
                }
            }
        }
        else if( info->isEnum() ) {
            auto strIval = common::iValueCast<common::StringValue>( value.get() );
            if( !strIval ) {
                return std::make_tuple( errors::InvalidParams, errors::data::WrongType );
            }
            if( !info->checkEnumValue( strIval->get() ) ) {
                return std::make_tuple( errors::InvalidParams, errors::data::RangeInvalid );
            }
        }

        itr->second.emplace_back( itemId );
    }

    for( auto& val : valueToItems ) {
        if( !bus::command::CloudItemValueSet::send( "debug", m_client, std::move( val.second ), val.first->clone() ) ) {
            return std::make_tuple( errors::InternalError, StringType() );
        }
    }

    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace nma
