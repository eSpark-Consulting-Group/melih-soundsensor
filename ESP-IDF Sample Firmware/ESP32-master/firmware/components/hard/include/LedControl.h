
#pragma once

#include "LedDevice.h"
#include "Shutdownable.h"

#include <cstdint>
#include <memory>
#include <mutex>

namespace hard {

class LedControl : public core::SingletonBaseImpl<LedControl> {
    friend class core::SingletonBaseImpl<LedControl>;

    using ParentType = core::SingletonBaseImpl<LedControl>;
    using ClbkRet = ParentType::ClbkRet;

    enum class State : std::uint8_t {
        IDLE,
        SOFT_RESET_LD,
        HARD_RESET_LD,
        FactoryDefaults_LD,
        ExcludeInvoked_LD,
        IncludeInvoked_LD,
        INCLUDE_LD,
        EXCLUDE_LD,
        STOP_INCLUDE_LD,
        STOP_EXCLUDE_LD,
        resetWiFi_LD,
        NON_WIFI_LD,
        ServerDisconn_LD
    };

public:
    static inline void onStartHardReset() {
        auto instance = get();
        if( instance ) {
            instance->processStartHardReset();
        }
    }

    static inline void onStartSoftReset() {
        auto instance = get();
        if( instance ) {
            instance->processStartSoftReset();
        }
    }

    static inline void onResetToFactoryDefaults() {
        auto instance = get();
        if( instance ) {
            instance->processResetToFactoryDefaults();
        }
    }

    static inline void onResetWifi() {
        auto instance = get();
        if( instance ) {
            instance->processResetWifi();
        }
    }

    static inline void onSendNif() {
        auto instance = get();
        if( instance ) {
            instance->processSendNif();
        }
    }

    static inline void onIndicatorIdentify() {
        auto instance = get();
        if( instance ) {
            instance->processIndicatorIdentify();
        }
    }

#ifdef CONFIG_BOARD_TYPE_PLUG_HUB
    static inline void onPlugSwitchOn( const bool isOn ) {
        auto instance = get();
        if( instance ) {
            instance->processPlugOn( isOn );
        }
    }
#endif  // CONFIG_BOARD_TYPE_PLUG_HUB

    void setIsWifiConfigExist( const bool isWifiConfExist );

private:
    using ParentType::SingletonBaseImpl;

    bool initializeImpl() override;

    ClbkRet updateComponent( UpdContext& ctx ) override {
        return ClbkRet::Continue;
    }

    void visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::HubSendBroadcast& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const cloud_client::responces::ConnectedToCloud& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const cloud_client::responces::DisconnectedFromCloud& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const network::wifi_srv2::responce::ConnectedToWifiStation& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const network::wifi_srv2::responce::DisconnectedFromWifiStation& command ) override;

    inline auto& led() {
        return *m_led;
    }

    inline auto currState() const {
        return m_currState;
    }

    void setState( const State newState );

    void processStartHardReset();

    void processStartSoftReset();

    void processResetToFactoryDefaults();

    void processResetWifi();

    void processLostWifi();

    void processConnectedToWifi();

    void processDisconnectedFromCloud();

    void processConnectedToCloud();

    void processSendNif();

    void processIndicatorIdentify();

    void processExcludeInvoked();

    void processExcludeStarted();

    void processExcludeStopped();

    void processIncludeInvoked();

    void processIncludeStarted();

    void processIncludeStopped();

#ifdef CONFIG_BOARD_TYPE_PLUG_HUB
    void processPlugOn( const bool isOn );
#endif  // CONFIG_BOARD_TYPE_PLUG_HUB;

private:
    std::mutex m_mutex;
    State m_currState = State::IDLE;

    std::unique_ptr<LedDevice> m_led;

#ifdef CONFIG_BOARD_TYPE_PLUG_HUB
    bool m_plugSwitchisOn = false;
#endif  // CONFIG_BOARD_TYPE_PLUG_HUB;
};

}  // namespace hard