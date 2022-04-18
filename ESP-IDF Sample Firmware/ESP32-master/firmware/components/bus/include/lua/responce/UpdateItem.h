#pragma once

#include <string>
#include <memory>
#include <IValue.h>
#include <BusCommand.h>
#include <DeviceInfo.h>

namespace lua {
namespace responce {

class UpdateItem final : public bus::IBusCommand {
public:
    UpdateItem( gateways::ItemInfo::UPtr itemInfo, common::IValue::UPtr value )
    : m_itemInfo( std::move( itemInfo ) )
    , m_value( std::move( value ) ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) {
        client.visit( from, to, *this );
    }

    inline const auto& itemInfo() {
        return *m_itemInfo;
    }

    inline auto value() const {
        return m_value ? m_value->clone() : nullptr;
    }

    inline bool isValueValid() const {
        return bool( m_value );
    }

    gateways::ItemInfo::UPtr m_itemInfo;
    common::IValue::UPtr m_value;
};

}  // namespace responce
}  // namespace lua
