#pragma once

#include <BusCommand.h>

#include <list>
#include <string>
#include <functional>

namespace common {
class IWriteStream;
}  // namespace common

namespace bus {

class IBusClient;

namespace command {
namespace nma {

static constexpr bool ResponceCmdWasntHandled = false;

class[[deprecated]] ClientClbk final : public bus::BusCommandResp<bool, ResponceCmdWasntHandled> {
public:
    using ParentType = bus::BusCommandResp<bool, ResponceCmdWasntHandled>;
    using ClbkType = std::function<bool( common::IWriteStream& )>;

    ClientClbk( ClbkType && clbk )
    : m_clbk( std::move( clbk ) ) {}

    inline auto& getClbk() {
        return m_clbk;
    }

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    using ParentType::setResponce;

    inline bool executeClbk( common::IWriteStream & wStream ) {
        bool isSuccess = getClbk()( wStream );
        setResponce( isSuccess );
        return isSuccess;
    }

    inline static bool createExecTimeoutErrorResult() {
        return false;
    }

private:
    ClbkType m_clbk;
};

}  // namespace nma
}  // namespace command
}  // namespace bus
