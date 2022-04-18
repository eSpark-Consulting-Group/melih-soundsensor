#pragma once

#include <BusCommand.h>
#include <string.h>
#include <memory>
#include <znet/znet_command.hpp>

namespace bus {
namespace command {

class CreateChipBackup final : public IBusCommand {
public:
    CreateChipBackup(::znet::ModeStatus status ) {
        m_status = status;
    }

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    ::znet::ModeStatus getStatus() {
        return m_status;
    }

private:
    ::znet::ModeStatus m_status;
};

}  // namespace command
}  // namespace bus
