#pragma once

#include "BusCommand.h"

namespace bus {
namespace command {

class GpioButtonClicked : public IBusCommand {
public:
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

class GpioButtonDoubleClicked : public IBusCommand {
public:
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

class GpioButtonLongClicked : public IBusCommand {
public:
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

class GpioButtonVeryLongClicked : public IBusCommand {
public:
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

class GpioButtonVeryVeryLongClicked : public IBusCommand {
public:
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

}  // namespace command
}  // namespace bus
