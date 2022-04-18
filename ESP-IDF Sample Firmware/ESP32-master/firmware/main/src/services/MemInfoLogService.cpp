#include "services/MemInfoLogService.h"

#include "Utils.h"

namespace sys_serv {

namespace {

void tryShowAverage() {
    constexpr auto average_update_period = std::chrono::seconds( 10 );
    static auto average_print_timepoint = std::chrono::steady_clock::now() + average_update_period;

    if( std::chrono::steady_clock::now() < average_print_timepoint ) {
        return;
    }

    average_print_timepoint = std::chrono::steady_clock::now() + average_update_period;
    common::printSystemAverage();
}

}  // namespace

MemInfoLogService::ClbkRet MemInfoLogService::updateComponent( UpdContext& ctx ) {
    tryShowAverage();

    return ClbkRet::Continue;
}

}  // namespace sys_serv
