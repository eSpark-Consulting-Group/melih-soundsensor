#pragma once

#include "BusCommand.h"
#include "IValueCast.h"
#include "GatewayID.h"

namespace bus::command {

class UpdateGateway : public IBusCommand {
public:
    UpdateGateway(::gateways::GatewayID gatewayId, std::unique_ptr<::common::TableValue> updatedFields )
    : m_gatewayId( gatewayId )
    , m_updatedFields( std::move( updatedFields ) ) {}

    UpdateGateway(::gateways::GatewayID gatewayId,
                  const std::string& fieldName,
                  std::unique_ptr<::common::IValue> fieldValue )
    : m_gatewayId( gatewayId )
    , m_updatedFields( std::make_unique<::common::TableValue>() ) {
        m_updatedFields->insert( fieldName, std::move( fieldValue ) );
    }

    auto getGatewayId() const {
        return m_gatewayId;
    }

    std::unique_ptr<::common::TableValue> getUpdatedFields() const {
        return ::common::iValueCast<::common::TableValue>( m_updatedFields->clone() );
    }

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

private:
    ::gateways::GatewayID m_gatewayId;
    std::unique_ptr<::common::TableValue> m_updatedFields;
};

}  // namespace bus::command
