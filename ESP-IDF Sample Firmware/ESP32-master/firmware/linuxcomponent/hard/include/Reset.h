#include <cstdint>

namespace hard {

class Reset {
    enum class ResetType { Undefined, SoftReset, ResetToFactoryDefaults, ResetFirmware };

    static constexpr std::uint16_t MinFixTime = 4;
    static constexpr std::uint16_t ShortFixTime = 100;
    static constexpr std::uint16_t LongFixTime = 3000;
    static constexpr std::uint16_t VeryLongFixTime = 10000;
    static constexpr std::uint16_t TimeOut = 5000;

    using resetCallback_t = void ( * )( void );

public:
    static Reset& get();

    ~Reset() = default;

    bool initialize();
    void check();

    void setCallbackBroadcast( resetCallback_t cb );
    void setCallbackResetWifi( resetCallback_t cb );
    void setCallbackSoftReset( resetCallback_t cb );
    void setCallbackResetToFactoryDefaults( resetCallback_t cb );
    void setCallbackUseOtherFirmware( resetCallback_t cb );

private:
    bool m_initialized;
    ResetType m_resetType;
    int m_frontTime;
    int m_pressedTime;
    uint8_t m_shorts;
    resetCallback_t m_broadcast = nullptr;
    resetCallback_t m_resetWifi = nullptr;
    resetCallback_t m_softReset = nullptr;
    resetCallback_t m_resetToFactoryDefaults = nullptr;
    resetCallback_t m_useOtherFirmware = nullptr;

private:
    Reset();
    void setDefaultState();
    void handleButtonPressed( std::int64_t time, bool level );
    static void gpioCallback( void* arg );
};

}  // namespace hard
