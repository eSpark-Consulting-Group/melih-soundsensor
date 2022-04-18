#include <gason.hpp>

#include "gateways/commands/BusCommandZwaveProvisioningList.h"

#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "CommonResources.h"
#include "GatewayUtils.h"

namespace nma {

using namespace std::chrono_literals;
namespace errors = common::resources::errors;
namespace tokens = common::resources::tokens;
namespace provisioning = common::resources::provisioning;

bool parseNodeIdList( const gason::JsonValue& listJson,
                      std::unordered_set<uint8_t>& result,
                      WebSocketHandler::ResponceType& error ) {
    if( listJson ) {
        if( !listJson.isArray() ) {
            error = { errors::InvalidParams, "The 'nodeIds' node must be an array of node IDs." };
            return false;
        }
        for( const auto& idJson : listJson ) {
            if( !idJson.value.isNumber() ) {
                error = { errors::InvalidParams,
                          "Invalid type of nodeIds item [" + std::to_string( result.size() ) +
                              "], expected numeric value." };
                return false;
            }
            result.insert( idJson.value.toInt() );
        }
    }
    return true;
}

bool parseProvisioningIdList( const gason::JsonValue& listJson,
                              std::unordered_set<uint32_t>& result,
                              WebSocketHandler::ResponceType& error ) {
    if( listJson ) {
        if( !listJson.isArray() ) {
            error = { errors::InvalidParams, "The 'ids' node must be an array of IDs of provisioning list records." };
            return false;
        }
        for( const auto& provisioningIdJson : listJson ) {
            if( !provisioningIdJson.value.isString() ) {
                error = { errors::InvalidParams,
                          "Invalid type of ids item [" + std::to_string( result.size() ) +
                              "], expected string value." };
                return false;
            }
            result.insert( std::strtoul( provisioningIdJson.value.toString(), nullptr, 10 ) );
        }
    }
    return true;
}

WebSocketHandler::ResponceType WebSocketHandler::onHubAddonsZwaveNodesProvisioningList(
    const gason::JsonValue& input,
    common::JsonStreamGenerator& gen ) {
    bool started = false;
    auto entryProcessor = [&started, &gen]( const ::znet::ProvisioningListEntry& entry ) {
        if( !started ) {
            started = true;
            if( !gen.writeOpeningObject() || !gen.writePropertyKey( provisioning::Provisioning ) ||
                !gen.writeOpeningArray() ) {
                return true;
            }
        }
        return !gen.writeOpeningObject() ||
               !gen.writeProperty( tokens::SubId, std::to_string( entry.m_provisioningId ) ) ||
               !gen.writeProperty( provisioning::Dsk, entry.m_dsk ) ||
               !::gateways::utils::writeProvisioningListEntryFields( gen, entry.m_fields ) || !gen.writeClosingSymbol();
    };
    WebSocketHandler::ResponceType errorResponse;
    std::unordered_set<uint8_t> nodeIdList;
    std::unordered_set<uint32_t> provisioningIdList;
    if( !parseNodeIdList( input( tokens::Params )( provisioning::NodeIds ), nodeIdList, errorResponse ) ||
        !parseProvisioningIdList(
            input( tokens::Params )( provisioning::ProvisioningIds ), provisioningIdList, errorResponse ) ) {
        gen.writeEmptyObject() && gen.writeItemDelimiter();
        return errorResponse;
    }
    auto future = m_client.asyncSend<::gateways::commands::ZwaveProvisioningList>(
        "znet", entryProcessor, std::move( nodeIdList ), std::move( provisioningIdList ) );
    if( future.wait_for( 10s ) == std::future_status::timeout ) {
        gen.writeEmptyObject() && gen.writeItemDelimiter();
        return { errors::ApplicationError, "The operation has timed out." };
    }
    else {
        auto status = future.get();
        if( status != ::znet::api::IProvisioningListController::OperationStatus::SUCCESS ) {
            gen.writeEmptyObject() && gen.writeItemDelimiter();
            return { errors::ApplicationError,
                     "Z-Wave module has reported an error: " + std::to_string( static_cast<int>( status ) ) };
        }
        else if( !started ) {
            gen.writeOpeningObject() && gen.writePropertyKey( provisioning::Provisioning ) && gen.writeOpeningArray() &&
                gen.writeClosingSymbol() && gen.writeClosingSymbol();
            return { errors::NotError, std::string{} };
        }
        else {
            gen.writeClosingSymbol();  // close array element that we opened when processing first entry
            gen.writeClosingSymbol();
            return { errors::NotError, std::string{} };
        }
    }
}

}  // namespace nma
