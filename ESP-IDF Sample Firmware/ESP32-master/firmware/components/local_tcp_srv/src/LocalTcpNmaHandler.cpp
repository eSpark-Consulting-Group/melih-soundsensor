#include "LocalTcpNmaHandler.h"

#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

namespace local_tcp_srv {

using namespace common::resources;

LocalTcpNmaHandler::ResponceType LocalTcpNmaHandler::onRegisterNmaClient( const gason::JsonValue& json,
                                                                          common::JsonStreamGenerator& gen ) {
    bool res = gen.writeOpeningObject();
    {
        res = res && gen.writeProperty( tokens::Api, "1" );
        res = res && gen.writeProperty( tokens::Id, "_id" );
        res = res && gen.writeProperty( tokens::Method, commands::Registered );
    }
    res = res && gen.writeClosingSymbol();
    res = res && gen.writeJsonEnd();

    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace local_tcp_srv