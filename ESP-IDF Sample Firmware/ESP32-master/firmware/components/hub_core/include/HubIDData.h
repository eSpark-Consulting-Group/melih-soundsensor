#pragma once
#include <cstdint>

namespace core {

class HubIDData {
public:
    virtual ~HubIDData() = default;

    // Ver 1
    virtual std::uint64_t id() const = 0;
    virtual const char* uuid() const = 0;
    virtual const char* zwaveRegion() const = 0;
    virtual const char* defaultWiFiSSID() const = 0;
    virtual const char* defaultWiFiPass() const = 0;

    // Ver 2
    virtual const char* name() const = 0;
};

}  // namespace core
