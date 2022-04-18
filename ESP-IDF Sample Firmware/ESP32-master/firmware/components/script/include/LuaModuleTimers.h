#pragma once

#include <LuaModule.h>
#include <list>
#include <string>
#include <chrono>

#include "values/TableValue.h"

namespace lua {

class Executor;

namespace module {

class Timers final : public IModule {
public:
    typedef struct {
        std::string m_handlerScriptName;
        std::unique_ptr<::common::TableValue> m_handlerScriptArgs;
        std::chrono::milliseconds m_duration;
        bool m_isSingleShoot;
        std::chrono::time_point<std::chrono::system_clock> m_startTime;
    } Timer_t;

    Timers( Executor& exec, const char* name );

    bool initialize() override;

    int regModule( lua_State* lua ) override;

    void update();

    std::optional<std::string> createTimer( std::string handlerScriptName,
                                            std::unique_ptr<::common::TableValue> args,
                                            std::chrono::milliseconds duration,
                                            bool isSingleShoot );

    bool createTimer( std::string timerId,
                      std::string handlerScriptName,
                      std::unique_ptr<::common::TableValue> handlerScriptArgs,
                      std::chrono::milliseconds duration,
                      bool isSingleShoot );
    bool removeTimer( const std::string& timerId );

    bool isTimerExist( const std::string& _id ) const;
    std::unique_ptr<::common::TableValue> getHandlerScriptArgs( const std::string& timerId ) const;

    bool setHandlerScriptArgs( const std::string& timerId, std::unique_ptr<::common::TableValue> args );
    bool resetTimer( const std::string& timerId, std::chrono::milliseconds duration );

private:
    std::optional<std::string> generateTimerId() const;

private:
    std::map<std::string, Timer_t> m_timers;
};

}  // end of namespace module

}  // end of namespace lua
