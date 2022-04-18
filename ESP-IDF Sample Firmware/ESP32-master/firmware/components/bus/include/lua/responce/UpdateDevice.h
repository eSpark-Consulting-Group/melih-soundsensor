#pragma once

#include <string>
#include <memory>
#include <BusCommand.h>
#include <IValue.h>

namespace lua {
namespace responce {

struct UpdateDevice final : public bus::IBusCommand {
    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    std::string m_deviceId;
    std::string m_key;
    common::IValue::UPtr m_value;
};

}  // namespace responce
}  // namespace lua
