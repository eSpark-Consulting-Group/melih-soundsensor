#pragma once

#include <cstdint>

namespace hard {

class Wifi {
public:
    enum class Mode {
        sta, /**< WiFi station mode */
        ap,  /**< WiFi soft-AP mode */
    };

    enum class AuthenticateMode {
        open = 0,        /**< AUTHENTICATE MODE : OPEN */
        wep,             /**< AUTHENTICATE MODE : WEP */
        wpa_psk,         /**< AUTHENTICATE MODE : WPA_PSK */
        wpa2_psk,        /**< AUTHENTICATE MODE : WPA2_PSK */
        wpa_wpa2_psk,    /**< AUTHENTICATE MODE : WPA_WPA2_PSK */
        wpa2_enterprise, /**< AUTHENTICATE MODE : WPA2_ENTERPRISE */
        max,
        error
    };

    enum class DisconnectReason { auth_fail, no_ap_found, beacon_timeout, handshake_timeout, assoc_fail, undefined };

    enum class Event { ap_stadisconnected, sta_disconnected, sta_connected, sta_got_ip, undefined };

    struct Config {
        char ssid[33];
        uint8_t bssid[6];
        char password[64];
    };

    using EventArg = void*;
    using EventCallback = bool ( * )( EventArg arg, Event event );

    struct EventData {
        EventCallback eventCallback;
        EventArg eventArg;
        DisconnectReason disconnectReason;
    };

public:
    static Wifi& get();

public:
    bool init();
    void deinit();
    bool startWifi();
    bool getMac( std::uint8_t dst[6] ) const;
    bool getIpAddr( char* str, std::size_t maxlen ) const;
    bool startAp( const char* ssid, AuthenticateMode authenticateMode, std::uint8_t maxClientCount ) const;
    bool scanStart( std::uint32_t scanTimeout ) const;
    bool setMode( Mode mode ) const;
    bool setConfig( Mode mode, const Config& config ) const;
    bool getConfig( Mode mode, Config* config ) const;
    bool connect();
    bool disconnect() const;
    bool restart() const;
    bool initEventLoop( EventCallback cb, void* arg );
    DisconnectReason getDisconnectReason() const;

private:
    EventData m_eventData;
    bool m_isInitialized;
    Wifi()
    : m_isInitialized( false ){};
    Wifi( const Wifi& ) = delete;
    Wifi& operator=( const Wifi& ) = delete;
};

}  // namespace hard
