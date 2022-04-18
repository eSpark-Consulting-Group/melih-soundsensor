#include "Shutdownable.h"

#include "DebugUtils.h"

namespace core {

bool Shutdownable::initializeImpl() {
    return bus::Bus::get().join( commands::BUS_NAME, *this );
}

Shutdownable::ClbkRet Shutdownable::updateShutdownState() {
    if( isShutdowned() ) {
        return ClbkRet::Completed;
    }

    if( ShutdownState::ShutdownStart == m_shutdownState && ClockType::now() - m_timepoint > m_shutdownDelay ) {
        m_shutdownState = ShutdownState::Shutdowned;
        leaveBus();
        onShutdownComplete();
        return ClbkRet::Completed;
    }

    return ClbkRet::Continue;
}

Shutdownable::ClbkRet Shutdownable::update( UpdContext& ctx ) {
    if( ClbkRet::Completed == updateShutdownState() ) {
        return ClbkRet::Completed;
    }

    tryReceiveSingleMessage();

    return ParentType::update( ctx );
}

void Shutdownable::startShutdown() {
    m_shutdownState = ShutdownState::ShutdownStart;
    m_timepoint = ClockType::now();
    onShutdownStart();
}

void Shutdownable::visit( const bus::BusAddr from, const bus::BusAddr to, const commands::Shutdown& command ) {
    if( ShutdownState::ShutdownStart == m_shutdownState || ShutdownState::Shutdowned == m_shutdownState ) {
        FUNC_LOGW( "Component already shutdowned!" );
        return;
    }

    startShutdown();
}

}  // namespace core
