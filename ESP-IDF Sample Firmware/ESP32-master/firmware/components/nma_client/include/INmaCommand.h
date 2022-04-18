#pragma once

namespace common {
class Buffer;
}  // namespace common

namespace nma {

class NmaSenderInfo;

class ICommand {
public:
    virtual ~ICommand() = default;

public:
    virtual bool execute() = 0;

    virtual bool processActiveToken() = 0;
    virtual bool onStepIn() = 0;
    virtual bool onStepOut() = 0;
    virtual void onSymbolParsed( char symbol ) = 0;
    virtual bool onLongToken() = 0;
};

}  // namespace nma
