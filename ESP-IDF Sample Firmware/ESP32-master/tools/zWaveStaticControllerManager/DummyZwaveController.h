#pragma once

class DummyZwaveControllerImp;

//-----------------------------------------------------------------------------
// DummyZwaveController
//-----------------------------------------------------------------------------

class DummyZwaveController {
public:
    DummyZwaveController();
    ~DummyZwaveController();

    bool step();

private:
    DummyZwaveControllerImp* m_imp = nullptr;
};
