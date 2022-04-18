#pragma once

#include "GatewayID.h"

namespace gateways {

/**
 * Provides access to additional gateway data fields used by Lua to manage gateways.
 */
class IGatewayUserDataStorage {
public:
    virtual ~IGatewayUserDataStorage() = default;

    /**
     * Stores given fields for the specific gateway.
     * Other fields not mentioned in @param fields won't be affected.
     */
    virtual void setUserData(::gateways::GatewayID, std::unique_ptr<::common::TableValue> fields ) = 0;

    /**
     * Retrieves fields for the specified gateway.
     * If no fields available, returns nullptr.
     */
    virtual std::unique_ptr<::common::TableValue> getUserData(::gateways::GatewayID ) const = 0;
};

}  // namespace gateways
