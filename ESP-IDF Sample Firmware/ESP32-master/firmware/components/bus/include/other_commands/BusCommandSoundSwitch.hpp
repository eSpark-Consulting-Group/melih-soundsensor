#pragma once

#include "Bus.h"
#include "BusCommandHandleEvent.h"

namespace bus {
namespace command {

/**
 * Base for COMMAND_CLASS_SOUND_SWITCH commands.
 */
class SoundSwitchCommand {
public:
    SoundSwitchCommand( uint8_t nodeId, uint8_t channelId )
    : m_nodeId( nodeId )
    , m_channelId( channelId ) {}

    uint8_t getNodeId() const {
        return m_nodeId;
    }

    uint8_t getChannelId() const {
        return m_channelId;
    }

private:
    uint8_t m_nodeId;
    uint8_t m_channelId;
};

/**
 * Base for COMMAND_CLASS_SOUND_SWITCH commands with tone/volume params.
 */
class SoundSwitchCommandWithArgs : public SoundSwitchCommand {
public:
    SoundSwitchCommandWithArgs( uint8_t nodeId, uint8_t channelId, uint8_t toneId, uint8_t volume )
    : SoundSwitchCommand( nodeId, channelId )
    , m_toneId( toneId )
    , m_volume( volume ) {}

    uint8_t getToneId() const {
        return m_toneId;
    }

    uint8_t getVolume() const {
        return m_volume;
    }

private:
    uint8_t m_toneId;
    uint8_t m_volume;
};

class SoundSwitchConfigrationGet : public SoundSwitchCommand, public IBusCommand {
public:
    using SoundSwitchCommand::SoundSwitchCommand;
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

class SoundSwitchTonePlayGet : public SoundSwitchCommand, public IBusCommand {
public:
    using SoundSwitchCommand::SoundSwitchCommand;
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

class SoundSwitchConfigrationSet : public SoundSwitchCommandWithArgs, public IBusCommand {
public:
    // Different order of Tone ID, Volume ID params in Lua stack for *_CONFIGURATION and *_TONE_PLAY commands.
    SoundSwitchConfigrationSet( uint8_t nodeId, uint8_t channelId, uint8_t volume, uint8_t defaultToneId )
    : SoundSwitchCommandWithArgs( nodeId, channelId, defaultToneId, volume ) {}

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

class SoundSwitchTonePlaySet : public SoundSwitchCommandWithArgs, public IBusCommand {
public:
    // Different order of Tone ID, Volume ID params in Lua stack for *_CONFIGURATION and *_TONE_PLAY commands.
    SoundSwitchTonePlaySet( uint8_t nodeId, uint8_t channelId, uint8_t toneId, uint8_t volume )
    : SoundSwitchCommandWithArgs( nodeId, channelId, toneId, volume ) {}

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

}  // namespace command
}  // namespace bus
