#include <DummyZwaveController.h>

#include <memory>

#include <StateFlow.h>
#include <ZwFaultState.h>
#include <ZwInitState.h>
#include <ZwRunState.h>

//-----------------------------------------------------------------------------
// DummyZwaveControllerImp
//-----------------------------------------------------------------------------

class DummyZwaveControllerImp {
public:
    DummyZwaveControllerImp();
    ~DummyZwaveControllerImp();

    bool step();

    std::unique_ptr<zwave_gateway::ZwStatesTable> m_states;
    std::unique_ptr<zwave_gateway::ZwTransitionsTable> m_transitions;
    std::unique_ptr<zwave_gateway::ZwStateMachine> m_zwg;

    std::unique_ptr<zwave_gateway::ZwInitState> m_stateInit;
    std::unique_ptr<zwave_gateway::ZwRunState> m_stateRun;
    std::unique_ptr<zwave_gateway::ZwFaultState> m_stateFault;

protected:
    bool init();
    bool destroy();
};

//-----------------------------------------------------------------------------

DummyZwaveControllerImp::DummyZwaveControllerImp()
: m_states( nullptr )
, m_transitions( nullptr )
, m_zwg( nullptr )
, m_stateInit( nullptr )
, m_stateRun( nullptr )
, m_stateFault( nullptr ) {
    init();
}

//-----------------------------------------------------------------------------

DummyZwaveControllerImp::~DummyZwaveControllerImp() {
    destroy();
}

//-----------------------------------------------------------------------------

bool DummyZwaveControllerImp::step() {
    return m_zwg->doStep();
}

//-----------------------------------------------------------------------------

bool DummyZwaveControllerImp::init() {
    m_states.reset( new zwave_gateway::ZwStatesTable() );
    m_transitions.reset( new zwave_gateway::ZwTransitionsTable() );

    m_zwg.reset( new zwave_gateway::ZwStateMachine( m_states.get(), m_transitions.get() ) );

    m_transitions->copyFrom( zwave_gateway::ZwStateMachine::getTransitions(),
                             zwave_gateway::ZwStateMachine::getTransitionsCount() );

    m_stateInit.reset( new zwave_gateway::ZwInitState( *m_zwg ) );
    m_stateRun.reset( new zwave_gateway::ZwRunState( *m_zwg ) );
    m_stateFault.reset( new zwave_gateway::ZwFaultState( *m_zwg ) );

    bool pushSuccess = true;
    pushSuccess = m_states->pushBack( m_stateInit.get() ) && pushSuccess;
    pushSuccess = m_states->pushBack( m_stateRun.get() ) && pushSuccess;
    pushSuccess = m_states->pushBack( m_stateFault.get() ) && pushSuccess;

    assert( pushSuccess );

    return m_zwg->create();
}

//-----------------------------------------------------------------------------

bool DummyZwaveControllerImp::destroy() {
    m_zwg.reset( nullptr );

    m_stateInit.reset( nullptr );
    m_stateRun.reset( nullptr );
    m_stateFault.reset( nullptr );

    m_transitions.reset( nullptr );
    m_states.reset( nullptr );

    return true;
}

//-----------------------------------------------------------------------------
// DummyZwaveController
//-----------------------------------------------------------------------------

DummyZwaveController::DummyZwaveController() {
    m_imp = new DummyZwaveControllerImp();
}

//-----------------------------------------------------------------------------

DummyZwaveController::~DummyZwaveController() {
    delete m_imp;
    m_imp = nullptr;
}

//-----------------------------------------------------------------------------

bool DummyZwaveController::step() {
    return m_imp->step();
}
