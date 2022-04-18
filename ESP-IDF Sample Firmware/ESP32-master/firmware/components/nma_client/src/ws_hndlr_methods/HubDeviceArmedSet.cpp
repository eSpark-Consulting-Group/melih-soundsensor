#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "HouseModesManager.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubDeviceArmedSet( const gason::JsonValue& json,
                                                                      common::JsonStreamGenerator& gen ) {
    gen.writeEmptyObject() && gen.writeItemDelimiter();

    auto params = json( common::resources::tokens::Params );
    bool ok;
    const char* device_id = params( tokens::SubId ).toString();
    bool armed = params( tokens::Armed ).toBool( &ok );
    if( !device_id || !ok ) {
        return std::make_tuple( errors::InvalidParams, StringType() );
    }

    auto& hmm = core::HouseModesManager::get();
    bool prev_state;
    if( core::HouseModesManager::Result::OK == hmm.setDeviceArmed( device_id, armed, &prev_state ) ) {
        return std::make_tuple( errors::NotError, StringType() );
    }
    else {
        return std::make_tuple( errors::InvalidRequest, StringType() );
    }
}

}  // namespace nma