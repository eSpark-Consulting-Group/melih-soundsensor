#pragma once

#include <string>
#include <vector>
#include <utility>
#include <memory>
#include <BusCommand.h>
#include <IValue.h>

namespace bus {
namespace command {

struct ModesChanged final : public IBusCommand {
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    std::string m_modeId;
    std::vector<std::pair<std::string, std::unique_ptr<common::IValue>>> m_properties;
};

}  // namespace command
}  // namespace bus
