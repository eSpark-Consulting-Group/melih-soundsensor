#include "NmaJsonHandler.h"

#if defined( CONFIG_CLOUD_DEVICES ) and !defined( CONFIG_UART_API )

#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "Bus.h"
#include "Ezlo.h"
#include "BusCloudControllerAbstractSetResponce.h"

#include <ColudDevicesManager.h>
#include <CloudDevicesStorage.h>

#include <tuple>
#include <chrono>

#include <DebugUtils.h>

namespace cloud_client {

using namespace common::resources;

NmaJsonHandler::ResponceType NmaJsonHandler::onCloudControllerAbstractSetResponse(
    const gason::JsonValue& json,
    common::JsonStreamGenerator& jsonGenerator ) {
    auto idCStr = json( tokens::Id ).toString();
    if( !idCStr ) {
        return std::make_tuple( errors::InvalidParams, errors::data::InternalUnhandledExceptionError );
    }

    auto result = json( tokens::Result );

    bool res = false;

    if( m_device_ids.size() ) {
        res = jsonGenerator.writeOpeningObject();
        res = res && jsonGenerator.writeProperty( tokens::Api, "1" );
        res = res && jsonGenerator.writeProperty( tokens::Id, core::Ezlo::uuid() );
        res = res && jsonGenerator.writeProperty( tokens::Method, "cloud.abstract_subscribe" );

        res = res && jsonGenerator.writePropertyKey( tokens::Params );
        res = res && jsonGenerator.writeOpeningObject();
        res = res && jsonGenerator.writePropertyKey( "subscribe" );
        res = res && jsonGenerator.writeOpeningArray();
        for( const auto& device_id : m_device_ids ) {
            res = res && jsonGenerator.writeProperty( nullptr, device_id.c_str() );
        }
        res = result && jsonGenerator.writeClosingSymbol();
        res = result && jsonGenerator.writeClosingSymbol();
        res = result && jsonGenerator.writeClosingSymbol();
        m_device_ids.clear();
    }
    else {
        auto devices = result( tokens::Devices );
        if( !devices.isObject() ) {
            return std::make_tuple( errors::InvalidParams, "\"devices\" isn't json object" );
        }

        bool isConversionSuccess;
        auto version = result( tokens::Version ).toInt( &isConversionSuccess );
        if( !isConversionSuccess ) {
            return std::make_tuple( errors::InvalidParams, "\"version\" isn't json int" );
        }

        for( auto device_node : devices ) {
            auto device = device_node.value;
            const auto id = device_node.key;

            m_device_ids.push_back( id );

            auto devicesInfo = std::make_unique<gateways::cloud::Device>( id );

            {
                const auto name = device( tokens::Name ).toString();

                if( !name ) {
                    return std::make_tuple( errors::InvalidParams, "\"name\" isn't json string" );
                }
                devicesInfo->setName( name );
            }

            {
                const auto type = device( tokens::Type ).toString();
                if( !type ) {
                    return std::make_tuple( errors::InvalidParams, "\"type\" isn't json string" );
                }
                devicesInfo->setType( type );
            }

            {
                const auto typeId = device( "device_type_id" ).toString();
                if( !typeId ) {
                    return std::make_tuple( errors::InvalidParams, "\"device_type_id\" isn't json string" );
                }
                devicesInfo->setTypeId( typeId );
            }

            {
                const auto category = device( tokens::Category ).toString();
                if( !category ) {
                    return std::make_tuple( errors::InvalidParams, "\"category\" isn't json string" );
                }
                devicesInfo->setCategory( category );
            }

            {
                const auto subCategory = device( tokens::SubCategory ).toString();
                if( !subCategory ) {
                    return std::make_tuple( errors::InvalidParams, "\"subcategory\" isn't json string" );
                }
                devicesInfo->setSubCategory( subCategory );
            }

            {
                const int battPow = device( "battery_powered" ).toInt( &isConversionSuccess );
                if( !isConversionSuccess ) {
                    return std::make_tuple( errors::InvalidParams, "\"battery_powered\" isn't json int" );
                }
                devicesInfo->setBatteryPowered( battPow == 1 );
            }

            devicesInfo->setDeviceId( id );

            {
                const auto security = device( tokens::Security ).toString();
                if( !security ) {
                    return std::make_tuple( errors::InvalidParams, "\"security\" isn't json string" );
                }
                devicesInfo->setSecurity( security );
            }

            {
                const auto vendor = device( "protocol" )( "vendor" ).toString();
                if( !vendor ) {
                    return std::make_tuple( errors::InvalidParams, "\"vendor\" isn't json string" );
                }
                gateways::cloud::Protocol protocol;
                protocol.vendor.assign( vendor );
                devicesInfo->setProtocol( protocol );
            }

            {
                const auto account = device( tokens::Info )( "account" ).toString();
                if( !account ) {
                    return std::make_tuple( errors::InvalidParams, "\"account\" isn't json string" );
                }
                gateways::cloud::Account info;
                info.account.assign( account );
                devicesInfo->setInfo( info );
            }

            {
                if( device( tokens::Reachable ) ) {
                    const int isReachable = device( tokens::Reachable ).toInt( &isConversionSuccess );
                    if( !isConversionSuccess ) {
                        return std::make_tuple( errors::InvalidParams, "\"reachable\" isn't json int" );
                    }
                    devicesInfo->setReachable( isReachable == 1 );
                }
            }

            {
                auto capabilities = device( tokens::Capabilities );
                if( !capabilities.isArray() ) {
                    return std::make_tuple( errors::InvalidParams, "\"capabilities\" isn't json array" );
                }

                gateways::cloud::Device::Capabilities capatibilitiesVec;
                for( auto capatibility_node : capabilities ) {
                    const auto cap = capatibility_node.value.toString();
                    if( !cap ) {
                        return std::make_tuple( errors::InvalidParams,
                                                "\"capabilities\" array value isn't json string" );
                    }
                    capatibilitiesVec.push_back( cap );
                }

                devicesInfo->setCapabilities( capatibilitiesVec );
            }

            if( !gateways::cloud::DevicesStorage::get().checkVersion( version ) ) {
                gateways::cloud::DevicesStorage::get().addDeviceFile( devicesInfo, version );
            }
        }

        {
            auto errorCode = json( tokens::Error )( tokens::Code ).toInt( &isConversionSuccess );
            const bool isSuccess = !isConversionSuccess || errorCode == 0;
            m_client.send<bus::command::nma::CloudControllerAbstractSetResponce>( "scenes", idCStr, isSuccess );
        }

        res = jsonGenerator.writeOpeningObject();

        res = res && jsonGenerator.writeProperty( tokens::Api, "1" );
        res = res && jsonGenerator.writeProperty( tokens::Id, core::Ezlo::uuid() );
        res = res &&
              jsonGenerator.writeProperty( tokens::Method, common::resources::commands::CloudControllerAbstractsList );
        res = res && jsonGenerator.writePropertyKey( tokens::Params );
        res = res && jsonGenerator.writeOpeningObject();

        res = res && jsonGenerator.writeProperty( tokens::Version, version );
        res = res && jsonGenerator.writeClosingSymbol();

        res = res && jsonGenerator.writeClosingSymbol();
    }
    return std::make_tuple( res ? errors::NotError : errors::GenJsonFailed, "" );
}

}  // namespace cloud_client

#endif  // defined(CONFIG_CLOUD_DEVICES) and !defined(CONFIG_UART_API)
