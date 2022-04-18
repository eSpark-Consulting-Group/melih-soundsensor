#pragma once

#include <string>
#include <BusCommand.h>

namespace bus {
namespace command {

class SetHandler final : public IBusCommand {
public:
    SetHandler( BusAddr addr )
    : m_addr( addr ) {}

    SetHandler( BusAddr addr, const std::string& handlerName )
    : m_addr( addr )
    , m_handlerName( handlerName ) {}

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline const auto& addr() const {
        return m_addr;
    }

    inline const auto& handlerName() const {
        return m_handlerName;
    }

    inline auto isSubscribe() const {
        return !handlerName().empty();
    }

private:
    BusAddr m_addr;
    std::string m_handlerName;
};

}  // namespace command
}  // namespace bus
