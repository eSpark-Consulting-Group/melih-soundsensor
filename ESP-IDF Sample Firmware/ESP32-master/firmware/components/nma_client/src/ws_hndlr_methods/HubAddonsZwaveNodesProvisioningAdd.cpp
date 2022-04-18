#include <gason.hpp>
#include <znet/znet.h>
#include <znet/iznet_provisioning_list_controller.hpp>
#include <znet/converters/security_modes_converter.hpp>

#include "gateways/commands/BusCommandZwaveProvisioningAdd.h"
#include "CommonResources.h"
#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"

namespace nma {

namespace errors = ::common::resources::errors;
namespace tokens = ::common::resources::tokens;
namespace provisioning = ::common::resources::provisioning;

using namespace std::chrono_literals;

bool parseDsk( const ::gason::JsonValue& dskJson, ::znet::DSK& dsk, ::nma::WebSocketHandler::ResponceType& error ) {
    if( dskJson && dskJson.isArray() ) {
        size_t i = 0;
        for( const auto& word : dskJson ) {
            if( i >= dsk.size() ) {
                error = { errors::InvalidParams, "DSK is too long." };
                return false;
            }
            if( !word.value.isNumber() ) {
                error = { errors::InvalidParams,
                          "Invalid type of DSK item [" + std::to_string( i ) + "], expected numeric value." };
                return false;
            }
            dsk[i++] = static_cast<uint16_t>( word.value.toInt() );
        }
        if( 0 == i ) {
            error = { errors::InvalidParams, "DSK must be an array of 8 integers." };
            return false;
        }
        return true;
    }
    return false;
}

znet::ProvisioningListEntryFields::AddingMode addingModeFromString( const std::string& addingMode ) {
    if( addingMode == "s2" )
        return znet::ProvisioningListEntryFields::AddingMode::S2;
    if( addingMode == "smart" )
        return znet::ProvisioningListEntryFields::AddingMode::SMART;

    return znet::ProvisioningListEntryFields::AddingMode::UNDEFINED;
}

bool parseAddingMode( const ::gason::JsonValue& fieldJson,
                      ::znet::utils::optional<::znet::ProvisioningListEntryFields::AddingMode>& fieldValue,
                      ::nma::WebSocketHandler::ResponceType& error ) {
    if( fieldJson ) {
        if( !fieldJson.isString() ) {
            error = { errors::InvalidParams, "Invalid type of addingMode field, expected string value." };
            return false;
        }

        fieldValue = addingModeFromString( fieldJson.toString() );
        return fieldValue.has_value();
    }
    return true;
}

bool parseSupportedAddingModes( const ::gason::JsonValue& fieldJson,
                                std::vector<::znet::ProvisioningListEntryFields::AddingMode>& fieldValue,
                                ::nma::WebSocketHandler::ResponceType& error ) {
    if( fieldJson ) {
        if( !fieldJson.isArray() ) {
            error = { errors::InvalidParams, "Invalid type of security modes field, expected array value." };
            return false;
        }
        uint8_t i = 0;
        fieldValue.clear();
        for( const auto& itemJson : fieldJson ) {
            if( !itemJson.value.isString() ) {
                error = { errors::InvalidParams,
                          "Invalid type of supported adding modes item [" + std::to_string( i ) +
                              "], expected string value." };
                return false;
            }

            ::znet::ProvisioningListEntryFields::AddingMode addingMode =
                addingModeFromString( std::string( itemJson.value.toString() ) );
            if(::znet::ProvisioningListEntryFields::AddingMode::UNDEFINED == addingMode ) {
                error = { errors::InvalidParams,
                          "Unknown value of supported adding modes item [" + std::to_string( i ) + "]." };
                return false;
            }

            fieldValue.push_back( addingMode );
            ++i;
        }
    }
    return true;
}

template <typename T>
bool parseIntField( const std::string&& name,
                    const ::gason::JsonValue& valueJson,
                    T& value,
                    ::nma::WebSocketHandler::ResponceType& error ) {
    if( !valueJson ) {
        error = { errors::InvalidParams, "Missing value of " + name + " field." };
        return false;
    }
    if( !valueJson.isNumber() ) {
        error = { errors::InvalidParams, "Invalid type of " + name + " field, expected numeric value." };
        return false;
    }
    value = static_cast<T>( valueJson.toInt() );
    return true;
}

bool parseProduct( const ::gason::JsonValue& fieldJson,
                   ::znet::utils::optional<::znet::ProvisioningListEntryFields::Product>& fieldValue,
                   ::nma::WebSocketHandler::ResponceType& error ) {
    if( fieldJson ) {
        fieldValue = ::znet::ProvisioningListEntryFields::Product{};
        return parseIntField(
                   "product.manufacturer", fieldJson( tokens::Manufacturer ), fieldValue->m_manufacturerId, error ) &&
               parseIntField( "product.type", fieldJson( tokens::Type ), fieldValue->m_productType, error ) &&
               parseIntField( "product.id", fieldJson( tokens::Id ), fieldValue->m_productId, error ) &&
               parseIntField( "product.version", fieldJson( tokens::Version ), fieldValue->m_version, error ) &&
               parseIntField( "product.subVersion", fieldJson( tokens::SubVersion ), fieldValue->m_subversion, error );
    }
    return true;
}

bool parseType( const ::gason::JsonValue& fieldJson,
                ::znet::utils::optional<::znet::ProvisioningListEntryFields::Type>& fieldValue,
                ::nma::WebSocketHandler::ResponceType& error ) {
    if( fieldJson ) {
        fieldValue = ::znet::ProvisioningListEntryFields::Type{};
        return parseIntField( "type.generic", fieldJson( provisioning::Generic ), fieldValue->m_generic, error ) &&
               parseIntField( "type.specific", fieldJson( provisioning::Specific ), fieldValue->m_specific, error ) &&
               parseIntField( "type.icon", fieldJson( provisioning::Icon ), fieldValue->m_icon, error );
    }
    return true;
}

bool parseMaxRequestInterval( const ::gason::JsonValue& fieldJson,
                              ::znet::utils::optional<int>& fieldValue,
                              ::nma::WebSocketHandler::ResponceType& error ) {
    if( fieldJson ) {
        int parsedValue;
        if( parseIntField( "maxRequestInterval", fieldJson( provisioning::Generic ), parsedValue, error ) ) {
            if( parsedValue < 640 || parsedValue > 12672 ) {
                error = { errors::InvalidParams,
                          "The value of maxRequestInterval must be in range of 640..12672 seconds." };
                return false;
            }
            if( parsedValue % 128 != 0 ) {
                error = { errors::InvalidParams, "The value of maxRequestInterval must be a multiple of 128." };
                return false;
            }
            fieldValue = parsedValue;
        }
    }
    return true;
}

bool parseUuid( const ::gason::JsonValue& fieldJson,
                ::znet::utils::optional<std::string>& fieldValue,
                ::nma::WebSocketHandler::ResponceType& error ) {
    if( fieldJson ) {
        if( !fieldJson.isString() || 0 == strlen( fieldJson.toString() ) ) {
            error = { errors::InvalidParams, "Invalid type of uuid field, expected a non-empty string value." };
            return false;
        }
        fieldValue = fieldJson.toString();
    }
    return true;
}

bool parseName( const ::gason::JsonValue& fieldJson,
                ::znet::utils::optional<std::string>& fieldValue,
                ::nma::WebSocketHandler::ResponceType& error ) {
    if( fieldJson ) {
        if( !fieldJson.isString() ) {
            error = { errors::InvalidParams, "Invalid type of name field, expected a non-empty string value." };
            return false;
        }
        auto fieldLength = std::strlen( fieldJson.toString() );
        if( fieldLength > 62 ) {
            error = { errors::InvalidParams, "The length of name field must be in range of 0..62." };
            return false;
        }
        fieldValue = fieldJson.toString();
    }
    return true;
}

bool parseRoom( const ::gason::JsonValue& fieldJson,
                ::znet::utils::optional<std::string>& fieldValue,
                ::nma::WebSocketHandler::ResponceType& error ) {
    if( fieldJson ) {
        if( !fieldJson.isString() || 0 == strlen( fieldJson.toString() ) ) {
            error = { errors::InvalidParams, "Invalid type of room field, expected a non-empty string value." };
            return false;
        }
        fieldValue = fieldJson.toString();
    }
    return true;
}

bool parseSecurityModes( const ::gason::JsonValue& fieldJson,
                         ::znet::utils::optional<uint8_t>& fieldValue,
                         ::nma::WebSocketHandler::ResponceType& error ) {
    if( fieldJson ) {
        if( !fieldJson.isArray() ) {
            error = { errors::InvalidParams, "Invalid type of security modes field, expected array value." };
            return false;
        }
        uint8_t i = 0, securityModesBitset = 0;
        for( const auto& itemJson : fieldJson ) {
            if( !itemJson.value.isString() ) {
                error = { errors::InvalidParams,
                          "Invalid type of security modes item [" + std::to_string( i ) + "], expected string value." };
                return false;
            }
            uint8_t modeBit = ::znet::security_modes_converter::strToType( itemJson.value.toString() );
            if( ZNET_NODEINFO_SECURITY_UNKNOWN == modeBit ) {
                error = { errors::InvalidParams,
                          "Unknown value of security modes item [" + std::to_string( i ) + "]." };
                return false;
            }
            securityModesBitset |= modeBit;
            ++i;
        }
        fieldValue = securityModesBitset;
    }
    return true;
}

void buildResponseFromOperationStatus(::nma::WebSocketHandler::ResponceType& response,
                                      ::znet::api::IProvisioningListController::OperationStatus status ) {
    switch( status ) {
    case ::znet::api::IProvisioningListController::OperationStatus::SUCCESS:
        response = { errors::NotError, std::string{} };
        break;
    default:
        response = { errors::ApplicationError,
                     "Z-Wave module has reported an error: " + std::to_string( static_cast<int>( status ) ) };
    }
}

WebSocketHandler::ResponceType WebSocketHandler::onHubAddonsZwaveNodesProvisioningAdd(
    const gason::JsonValue& input,
    common::JsonStreamGenerator& gen ) {
    gen.writeEmptyObject() && gen.writeItemDelimiter();

    WebSocketHandler::ResponceType response{ errors::ApplicationError, std::string{} };

    ::znet::DSK dsk;
    ::znet::ProvisioningListEntryFields fields;

    const auto& entry = input( tokens::Params );

    if( parseDsk( entry( provisioning::Dsk ), dsk, response ) &&
        parseName( entry( tokens::Name ), fields.m_name, response ) &&
        parseRoom( entry( provisioning::Room ), fields.m_room, response ) &&
        parseAddingMode( entry( provisioning::AddingMode ), fields.m_addingMode, response ) &&
        parseSupportedAddingModes(
            entry( provisioning::SupportedAddingModes ), fields.m_supportedAddingModes, response ) &&
        parseProduct( entry( provisioning::Product ), fields.m_product, response ) &&
        parseType( entry( tokens::Type ), fields.m_type, response ) &&
        parseUuid( entry( tokens::Uuid ), fields.m_uuid, response ) &&
        parseSecurityModes( entry( provisioning::SecurityModes ), fields.m_securityModes, response ) &&
        parseSecurityModes( entry( provisioning::SupportedModes ), fields.m_supportedModes, response ) &&
        parseMaxRequestInterval( entry( provisioning::MaxRequestInterval ), fields.m_maxRequestInterval, response ) ) {
        auto future = m_client.asyncSend<::gateways::commands::ZwaveProvisioningAdd>( "znet", dsk, fields );
        if( future.wait_for( 1s ) == std::future_status::timeout ) {
            response = { errors::ApplicationError, "The operation has timed out." };
        }
        else {
            buildResponseFromOperationStatus( response, future.get() );
        }
    }

    return response;
}

}  // namespace nma
