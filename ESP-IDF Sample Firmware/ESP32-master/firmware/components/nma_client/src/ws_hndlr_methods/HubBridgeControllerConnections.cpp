#ifdef CONFIG_SPIRAM

#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "CommonResources.h"
#include "ProvisionPeersInfoManager.h"
#include "gason.hpp"

#include <tuple>

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubBridgeControllerConnectionSet(
    const gason::JsonValue& json,
    common::JsonStreamGenerator& jsonGenerator ) {
    jsonGenerator.writeEmptyObject() && jsonGenerator.writeItemDelimiter();
    std::list<std::string> ipSet;
    const char* uuid = json( tokens::Params )( common::resources::tokens::SubId ).toString();
    const auto ipNode = json( tokens::Params )( common::resources::tokens::Ip );
    if( ipNode.isArray() ) {
        for( const auto& item : ipNode ) {
            const char* itemString = item.value.toString();
            if( itemString ) {
                ipSet.emplace_back( itemString );
            }
        }
    }

    auto client = provision::peers::InfoManager::get();
    if( !client ) {
        return std::make_tuple( errors::InternalError, StringType() );
    }

    if( client->addOrUpdatePeersIp( std::string( uuid ), ipSet ) ) {
        return std::make_tuple( errors::NotError, StringType() );
    }

    return std::make_tuple( errors::InternalError, StringType{} );
}

WebSocketHandler::ResponceType WebSocketHandler::onHubBridgeControllerConnectionList(
    const gason::JsonValue& json,
    common::JsonStreamGenerator& jsonGenerator ) {
    auto client = provision::peers::InfoManager::get();
    if( !client ) {
        return std::make_tuple( errors::InternalError, StringType() );
    }

    const auto peersInfoList = client->getPeersInfo();
    client.reset();

    bool result = jsonGenerator.writeOpeningObject();
    result = result && jsonGenerator.writePropertyKey( tokens::Controllers );
    result = result && jsonGenerator.writeOpeningArray();
    for( const auto& peerInfo : peersInfoList ) {
        result = result && jsonGenerator.writeOpeningObject();
        result = result && jsonGenerator.writeProperty( tokens::SubId, peerInfo.id() );
        result = result && jsonGenerator.writeProperty( tokens::Serial, peerInfo.serial() );
        result = result && jsonGenerator.writePropertyKey( tokens::Ip );
        result = result && jsonGenerator.writeOpeningArray();
        for( const auto& ip : peerInfo.ip() ) {
            result = result && jsonGenerator.writeProperty( nullptr, ip.c_str() );
        }
        result = result && jsonGenerator.writeClosingSymbol();
        result = result && jsonGenerator.writeProperty( tokens::Status, peerInfo.status() );
        result = result && jsonGenerator.writeClosingSymbol();
    }
    result = result && jsonGenerator.writeClosingSymbol();
    result = result && jsonGenerator.writeClosingSymbol();
    if( !result ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    return std::make_tuple( errors::NotError, std::string() );
}

}  // namespace nma

#endif  // CONFIG_SPIRAM