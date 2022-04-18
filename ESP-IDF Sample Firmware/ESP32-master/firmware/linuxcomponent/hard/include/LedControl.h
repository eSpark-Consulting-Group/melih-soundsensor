#pragma once

#include <cstdint>

namespace hard {

class LedDevice {
    static constexpr std::uint16_t ShortFixTime = 100;
    static constexpr std::uint16_t OneShortFixTime = 500;
    static constexpr std::uint16_t StartOnTime = 1000;
    static constexpr std::uint16_t LongOnTime = 5000;
    static constexpr std::uint16_t VeryLongOnTime = 10000;
    static constexpr std::uint16_t MaxDutyValue = 16384;
    static constexpr std::uint16_t QuarteDutyValue = 12288;
    static constexpr std::uint16_t HalfDutyValue = 8192;
    static constexpr std::uint16_t SlowFrequancy = 1;
    static constexpr std::uint16_t FastFrequancy = 10;
    static constexpr std::uint8_t LedOff = 1;
    static constexpr std::uint8_t LedOn = 0;

public:
    enum LEDControlType {
        LEDControlInit,
        LEDControlOff,
        LEDControlOn,
        LEDControlToggleSlow,
        LEDControlToggleFast,
        LEDControlStop
    };
    enum LEDControlSource { ResetSource, AnySource };

public:
    ~LedDevice() = default;
    static LedDevice& get();
    bool initialize();
    void check();
    void setFlashMode( LEDControlSource source,
                       LEDControlType ledcontrolType,
                       bool flashed = true,
                       int time = 0,
                       std::int8_t flashes = 1 );

private:
    void set( LEDControlType ledcontrolType );
    void setDefaultState();
    LedDevice();

private:
    bool m_initialized;
    LEDControlType m_ledcontrolType;
    LEDControlSource m_source;
    std::uint32_t m_flashTime;
    std::uint32_t m_startTime;
    std::uint8_t m_flashes;
};

}  // namespace hard
