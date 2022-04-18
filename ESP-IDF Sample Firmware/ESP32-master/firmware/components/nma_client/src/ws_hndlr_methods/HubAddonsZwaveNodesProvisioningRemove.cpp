#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "gateways/commands/BusCommandZwaveProvisioningRemove.h"
#include <DebugUtils.h>

namespace nma {

namespace tokens = common::resources::tokens;
namespace errors = common::resources::errors;
namespace provisioning = common::resources::provisioning;

using namespace std::chrono_literals;

WebSocketHandler::ResponceType buildResponseFromOperationStatus(
    ::znet::api::IProvisioningListController::OperationStatus status ) {
    switch( status ) {
    case ::znet::api::IProvisioningListController::OperationStatus::SUCCESS:
        return { errors::NotError, std::string{} };
    default:
        return { errors::ApplicationError,
                 "Z-Wave module error status: " + std::to_string( static_cast<int>( status ) ) };
    }
}

WebSocketHandler::ResponceType WebSocketHandler::onHubAddonsZwaveNodesProvisioningRemove(
    const gason::JsonValue& json,
    common::JsonStreamGenerator& gen ) {
    gen.writeEmptyObject() && gen.writeItemDelimiter();

    const auto& provisioning = json( tokens::Params )( provisioning::Provisioning );
    if( !provisioning.isArray() ) {
        return { errors::InvalidParams,
                 "The 'provisioning' node must be an array of IDs of provisioning list records to be removed." };
    }
    std::unordered_set<uint32_t> ids;
    for( const auto& word : provisioning ) {
        if( !word.value.isString() ) {
            return { errors::InvalidParams,
                     "Invalid type of ids item [" + std::to_string( ids.size() ) + "], expected string value." };
        };
        ids.insert( std::strtoul( word.value.toString(), nullptr, 10 ) );
    }
    auto future = m_client.asyncSend<::gateways::commands::ZwaveProvisioningRemove>( "znet", std::move( ids ) );
    if( future.wait_for( 1s ) == std::future_status::timeout ) {
        return { errors::ApplicationError, "The operation has timed out." };
    }
    else {
        return buildResponseFromOperationStatus( future.get() );
    }
}

}  // namespace nma
