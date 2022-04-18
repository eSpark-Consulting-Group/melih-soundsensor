#pragma once

#include <string>
#include <BusCommand.h>

namespace bus {
namespace command {
namespace nma {

class RequestTimeZones final : public IBusCommand {
public:
    RequestTimeZones( const std::string& timezoneId )
    : m_timezoneId( timezoneId ) {}

    const auto& timezoneId() const {
        return m_timezoneId;
    }

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

private:
    std::string m_timezoneId;
};

}  // namespace nma
}  // namespace command
}  // namespace bus
