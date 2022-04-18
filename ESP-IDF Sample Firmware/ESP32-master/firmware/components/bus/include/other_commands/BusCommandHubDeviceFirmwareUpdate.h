#pragma once

#include <BusCommand.h>
#include <IValue.h>

namespace bus {
namespace command {

class HubDeviceFirmwareUpdate final : public bus::IBusCommand {
public:
    HubDeviceFirmwareUpdate( common::IValue::UPtr&& updates )
    : m_updates( std::move( updates ) ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline const auto& updates() const {
        return m_updates;
    }

private:
    common::IValue::UPtr m_updates;
};

}  // namespace command
}  // namespace bus
