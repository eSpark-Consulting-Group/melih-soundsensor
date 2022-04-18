#pragma once 

#include "Shutdownable.h"

namespace sys_serv {

class MemInfoLogService : public core::Shutdownable {
public:
    using ParentType = core::Shutdownable;
    using ClbkRet = ParentType::ClbkRet;
    using UpdContext = core::upd_srv::UpdContext;

    using core::Shutdownable::Shutdownable;

    ClbkRet updateComponent( UpdContext& ctx ) override;
}; // class MemInfoLogService

} // namespace sys_serv