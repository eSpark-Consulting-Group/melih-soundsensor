#pragma once

#include <sdkconfig.h>
#if CONFIG_REMOTE_LOGGING

#include "Shutdownable.h"

#include <esp_log.h>

#include <cstdarg>

namespace core {

class Logger : public core::SingletonBaseImpl<Logger> {
    friend class core::SingletonBaseImpl<Logger>;

public:
    using ParentType = SingletonBaseImpl<Logger>;
    using ClbkRet = ParentType::ClbkRet;

    enum Level : unsigned int { ALL, TRACE, DEBUG, INFO, WARNING, ERROR, NONE };

    static constexpr auto MessageBufferSize = 1024;

    static const char* LevelToText( Level level );

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const cloud_client::responces::ConnectedToCloud& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const cloud_client::responces::DisconnectedFromCloud& command ) override;

    void write( const Level level, const char* tag, const char* format, ... );

    void setAllowed( bool value ) {
        m_isAllowed = value;
    }

    bool isAllowed() const {
        return m_isAllowed;
    }

    void setLogLevel( Level level ) {
        m_logLevel = level;
    }

    void setLogLevel( const char* level );
    Level getLogLevel();

private:
    using core::SingletonBaseImpl<Logger>::SingletonBaseImpl;

    Logger( const Logger& ) = delete;
    Logger& operator=( const Logger& ) = delete;

    static ::esp_log_level_t getEspLogLevel( Level );

    bool initializeImpl() override;

    ClbkRet updateComponent( UpdContext& ctx ) override {
        return ClbkRet::Continue;
    }

private:
    bool m_isAllowed = false;

    Level m_logLevel = Level( CONFIG_REMOTE_LOGLEVEL );
};

}  // namespace core

#endif  // CONFIG_REMOTE_LOGGING
