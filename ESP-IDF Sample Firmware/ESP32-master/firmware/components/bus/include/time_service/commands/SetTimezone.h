#pragma once

#include "BusCommand.h"

#include <string>
#include <optional>

namespace time_service {
namespace command {

class SetTimezone : public bus::IBusCommand {
public:
    SetTimezone( const std::string& timezoneId,
                 const std::string& timezoneDescr,
                 const std::optional<time_t>& expireTime = std::nullopt )
    : m_timezoneId( timezoneId )
    , m_timezoneDescr( timezoneDescr )
    , m_expireTime( expireTime ) {}

    inline const auto& timezoneId() const {
        return m_timezoneId;
    }

    inline const auto& timezoneDescr() const {
        return m_timezoneDescr;
    }

    inline const auto& expireTime() const {
        return m_expireTime;
    }

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

private:
    std::string m_timezoneId, m_timezoneDescr;
    std::optional<time_t> m_expireTime;
};

}  // namespace command
}  // namespace time_service
