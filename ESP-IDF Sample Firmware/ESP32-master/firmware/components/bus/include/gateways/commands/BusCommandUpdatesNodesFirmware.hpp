#pragma once

#include "BusCommand.h"
#include <values/TableValue.h>

namespace bus::command {

/**
 * Sends a list of nodes and a link to the firmware to the Firmware Update command class.
 */
class UpdatesNodesFirmware : public IBusCommand {
public:
    UpdatesNodesFirmware( std::vector<uint8_t> node_ids, std::unique_ptr<::common::ArrayValue> firmware )
    : m_node_ids( std::move( node_ids ) )
    , m_firmware( std::move( firmware ) ) {}

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
    inline const auto& node_ids() const {
        return m_node_ids;
    }

    inline const auto& firmware() const {
        return *m_firmware;
    }

private:
    std::vector<uint8_t> m_node_ids;
    std::unique_ptr<::common::ArrayValue> m_firmware;
};

}  // namespace bus::command
