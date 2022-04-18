
#pragma once

#include <cinttypes>

namespace gateways {

enum class RemoteButtEvents {
    Unknown = 0,

    Butt1ShortPress,
    Butt1LongPress,
    Butt2ShortPress,
    Butt2LongPress,
    Butt3ShortPress,
    Butt3LongPress,
    Butt4ShortPress,
    Butt4LongPress,
    Butt5ShortPress,
    Butt5LongPress,
    Butt6ShortPress,
    Butt6LongPress,
    Butt7ShortPress,
    Butt7LongPress,
    Butt8ShortPress,
    Butt8LongPress,
};

RemoteButtEvents convertStringToRemoteButtEvent( const char* event );

const char* toEnumValue( RemoteButtEvents event );

}  // namespace gateways
