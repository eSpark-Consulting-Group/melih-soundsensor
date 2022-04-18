#pragma once

#include <RemoteLogger.h>

#if defined( CONFIG_REMOTE_LOGGING )

#include <BusCommand.h>

namespace bus {
namespace command {

struct RemoteLog final : public IBusCommand {
    RemoteLog( const char* tag, const unsigned int timestamp, core::Logger::Level logLevel, std::string&& logText )
    : m_tag( tag )
    , m_timestamp( timestamp )
    , m_logLevel( logLevel )
    , m_logText( std::move( logText ) ) {}

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    std::string m_tag;
    unsigned int m_timestamp;
    core::Logger::Level m_logLevel;
    std::string m_logText;
};

}  // namespace command
}  // namespace bus

#endif  // CONFIG_REMOTE_LOGGING
