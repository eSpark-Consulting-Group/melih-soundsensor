#include "sdkconfig.h"

#ifndef CONFIG_UART_API

#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "wifi_service2/Common.h"
#include "Thread.h"
#include "Bus.h"
#include "syscore/commands/Shutdown.h"
#include "Shutdownable.h"
#include "BusCommandNmaClientClbk.h"
#include "wifi_service2/Common.h"

#include <memory>

#include "DebugUtils.h"

namespace cloud_client {
extern bool restartStartupService();
}  // namespace cloud_client

namespace nma {

namespace {

using namespace common::resources;
using namespace std::chrono_literals;

void restorePrevConnection( const network::wifi_srv2::StaConfig& prevConf, const bool isRunStartupService = true ) {
    if( !network::wifi_srv2::setStaConfig( prevConf ) ) {
        FUNC_LOGE( "Can't restore STA config" );
    }

    if( !network::wifi_srv2::joinToLastAccessPoint() ) {
        FUNC_LOGE( "Can't join to last AP" );
    }

    if( isRunStartupService && !cloud_client::restartStartupService() ) {
        FUNC_LOGE( "Can't restart cloud client's startup service" );
    }
}

void sendHubNetworkChangedSuccessBroadcast() {
    auto clbk = []( common::IWriteStream& ws ) -> bool {
        common::JsonStreamGenerator jsGen( ws );

        bool result = jsGen.writeOpeningObject();
        {
            result = jsGen.writeProperty( tokens::Id, broadcasts::Id );
            result = result && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubNetworkChanged );

            result = result && jsGen.writePropertyKey( tokens::Result );
            result = result && jsGen.writeOpeningObject();
            {
                result = result && jsGen.writeProperty( tokens::SyncNotification, true );

                result = result && jsGen.writePropertyKey( tokens::Interfaces );
                result = result && jsGen.writeOpeningArray();
                {
                    result = result && jsGen.writeOpeningObject();
                    {
                        result = result && jsGen.writeProperty( tokens::SubId, wifi::tokens::WLAN );

                        auto mac = network::wifi_srv2::MacType::get();
                        if( result && mac ) {
                            result = jsGen.writeProperty( tokens::Hwaddr, mac->toHexString() );
                        }

                        result = result && jsGen.writePropertyKey( tokens::Ipv4 );
                        result = result && jsGen.writeOpeningObject();
                        {
                            const auto ipInfo = network::wifi_srv2::getStaIpInfo();
                            result =
                                result && ipInfo && jsGen.writeProperty( tokens::Ip, ipInfo->getAddr().toString() );
                            result = result && ipInfo &&
                                     jsGen.writeProperty( tokens::Mask, ipInfo->getNetmask().toString() );
                            result = result && jsGen.writeProperty( tokens::Mode, "dhcp" );
                            result = result && jsGen.writeClosingSymbol();
                        }  // end of ipv4 block

                        result = result && jsGen.writeProperty( tokens::Status, "up" );

                        auto wifiApInfo = network::wifi_srv2::getStaApInfo();
                        if( wifiApInfo ) {
                            result = result && jsGen.writePropertyKey( "wifi" );
                            result = result && jsGen.writeOpeningObject();
                            {
                                result = result && jsGen.writePropertyKey( tokens::Network );
                                result = result && jsGen.writeOpeningObject();
                                {
                                    result = result && jsGen.writeProperty( tokens::Mode, "sta" );
                                    result = result && jsGen.writeProperty( "encryption", "psk2" );
                                    result = result && jsGen.writeProperty( tokens::Ssid, wifiApInfo->ssidToString() );
                                    result =
                                        result && jsGen.writeProperty( tokens::Bssid, wifiApInfo->bssidToHexString() );
                                    auto staConf = network::wifi_srv2::getStaConfig();
                                    if( staConf && result ) {
                                        result = jsGen.writeProperty( tokens::Key, staConf->passwordToString() );
                                    }
                                    result = result && jsGen.writeClosingSymbol();
                                }

                                result = result && jsGen.writeProperty( tokens::Region, "00" );
                                result = result && jsGen.writeClosingSymbol();
                            }
                        }
                        else {
                            result = result && jsGen.writeProperty( "wifi", nullptr );
                        }

                        result = result && jsGen.writeClosingSymbol();
                    }  // end of wlan0.1 block
                    result = result && jsGen.writeClosingSymbol();
                }  // end of interfaces array
                result = result && jsGen.writeClosingSymbol();
            }  // end of result block
            result = result && jsGen.writeClosingSymbol();
        }  // end of message block
        result = result && jsGen.writeClosingSymbol();

        return result;
    };

    if( !bus::sendFromNowhere<bus::command::nma::ClientClbk>( "nma", std::move( clbk ) ) ) {
        FUNC_LOGE( "can't send hub.network.changed broadcast" );
    }
}

void sendHubNetworkChangedFailedBroadcast( const int errorCode,
                                           const std::string& errorDescr,
                                           const std::string& ssid,
                                           const std::string& passw,
                                           const std::optional<network::wifi_srv2::BssidType>& bssid ) {
    auto clbk = [errorCode, errorDescr, ssid, passw, bssid]( common::IWriteStream& ws ) -> bool {
        common::JsonStreamGenerator jsGen( ws );

        bool result = jsGen.writeOpeningObject();
        {
            result = jsGen.writeProperty( tokens::Id, broadcasts::Id );
            result = result && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubNetworkChanged );

            result = result && jsGen.writePropertyKey( tokens::Result );
            result = result && jsGen.writeOpeningObject();
            {
                result = result && jsGen.writePropertyKey( tokens::Error );
                result = result && jsGen.writeOpeningObject();
                {
                    result = result && jsGen.writeProperty( tokens::Code, errorCode );
                    result = result && jsGen.writeProperty( tokens::Data, errorDescr );
                    result = result && jsGen.writeClosingSymbol();
                }  // end of error block

                result = result && jsGen.writePropertyKey( tokens::Interfaces );
                result = result && jsGen.writeOpeningArray();
                {
                    result = result && jsGen.writeOpeningObject();
                    {
                        result = result && jsGen.writeProperty( tokens::SubId, wifi::tokens::WLAN );

                        auto mac = network::wifi_srv2::MacType::get();
                        if( result && mac ) {
                            result = jsGen.writeProperty( tokens::Hwaddr, mac->toHexString() );
                        }

                        result = result && jsGen.writePropertyKey( "wifi" );
                        result = result && jsGen.writeOpeningObject();
                        {
                            result = result && jsGen.writePropertyKey( tokens::Network );
                            result = result && jsGen.writeOpeningObject();
                            {
                                result = result && jsGen.writeProperty( tokens::Mode, "sta" );
                                result = result && jsGen.writeProperty( "encryption", "psk2" );
                                result = result && jsGen.writeProperty( tokens::Ssid, ssid );
                                result = result && bssid && jsGen.writeProperty( tokens::Bssid, bssid->toHexString() );
                                result = result && jsGen.writeProperty( tokens::Key, passw );
                                result = result && jsGen.writeClosingSymbol();
                            }

                            result = result && jsGen.writeProperty( tokens::Region, "00" );
                            result = result && jsGen.writeClosingSymbol();
                        }

                        result = result && jsGen.writeClosingSymbol();
                    }  // end of wlan0.1 block
                    result = result && jsGen.writeClosingSymbol();
                }  // end of interfaces array
                result = result && jsGen.writeClosingSymbol();
            }  // end of result block
            result = result && jsGen.writeClosingSymbol();
        }  // end of message block
        result = result && jsGen.writeClosingSymbol();

        return result;
    };

    if( !bus::sendFromNowhere<bus::command::nma::ClientClbk>( "nma", std::move( clbk ) ) ) {
        FUNC_LOGE( "can't send hub.network.changed broadcast" );
    }
}

class TryConnectCommandExecutor : public core::Shutdownable {
public:
    using ClbkRet = core::Shutdownable::ClbkRet;
    using UpdContext = core::upd_srv::UpdContext;

    using ClockType = std::chrono::steady_clock;

    enum class State {
        Init,
        WaitingNmaClientsShutdown,
        JoiningToNewAccessPoint,
        WaitingConnectionToAp,
        WaitingConnectionToCloud,
        ConnectedToCloud,
        Completed,
        Failed
    };

    TryConnectCommandExecutor( std::string&& ssid,
                               std::string&& passw,
                               std::optional<network::wifi_srv2::BssidType>&& bssid,
                               network::wifi_srv2::StaConfig&& prevConf )
    : core::Shutdownable{ 0s }
    , m_ssid{ std::move( ssid ) }
    , m_passw{ std::move( passw ) }
    , m_bssid{ bssid }
    , m_prevConf{ std::move( prevConf ) } {}

    bool initializeImpl() override {
        return core::Shutdownable::initializeImpl() && bus::Bus::get().join( cloud_client::responces::BUS_NAME, *this );
    }

    ClbkRet updateComponent( UpdContext& ctx ) override {
        switch( m_state ) {
        case State::Init:
            if( !bus::sendFromNowhere<core::commands::Shutdown>( "nma" ) ) {
                setState( State::Completed );
                sendHubNetworkChangedFailedBroadcast(
                    errors::InternalError, errors::data::InternalOperationError, m_ssid, m_passw, m_bssid );
                return ClbkRet::Completed;
            }
            setState( State::WaitingNmaClientsShutdown );
            return setDelay( 2s );

        case State::WaitingNmaClientsShutdown:
            if( ClockType::now() - m_timePoint < 8s ) {
                return setDelay( 1s );
            }
            setState( State::JoiningToNewAccessPoint );
            [[fallthrough]];

        case State::JoiningToNewAccessPoint:
            if( !network::wifi_srv2::joinToAccessPoint( m_ssid, m_passw, m_bssid ) ) {
                FUNC_LOGE( "Can't join to AP ssid:%s, passw:%s", m_ssid.c_str(), m_passw.c_str() );
                m_errorCode = ( -32500 );
                m_errorDescr = errors::data::HubNetworkWifiConnectionFailed;
                restorePrevConnection( m_prevConf );
                setState( State::Failed );
                return ClbkRet::Continue;
            }
            setState( State::WaitingConnectionToAp );
            return setDelay( 4s );

        case State::WaitingConnectionToAp:
            if( network::wifi_srv2::isConnectedToAcessPoint() ) {
                if( !cloud_client::restartStartupService() ) {
                    FUNC_LOGE( "Can't restart cloud client's startup service" );
                    m_errorCode = errors::InternalError;
                    m_errorDescr = errors::data::InternalOperationError;
                    setState( State::Failed );
                    return ClbkRet::Continue;
                }
                setState( State::WaitingConnectionToCloud );
                return setDelay( 2s );
            }
            else if( ClockType::now() - m_timePoint > 20s ) {
                FUNC_LOGE( "Connection to AP ssid:%s, passw:%s failed", m_ssid.c_str(), m_passw.c_str() );
                m_errorCode = ( -32500 );
                m_errorDescr = errors::data::HubNetworkWifiConnectionFailed;
                restorePrevConnection( m_prevConf );
                setState( State::Failed );
                return ClbkRet::Continue;
            }
            else {
                return setDelay( 4s );
            }

        case State::WaitingConnectionToCloud:
            if( ClockType::now() - m_timePoint > 20s ) {
                FUNC_LOGE( "Can't connect to cloud with new access point" );
                m_errorCode = ( -32500 );
                m_errorDescr = errors::data::HubNetworkConnectionNoInternet;
                constexpr bool isRunStartupService{ false };
                restorePrevConnection( m_prevConf, isRunStartupService );
                setState( State::Failed );
                return ClbkRet::Continue;
            }
            else {
                return setDelay( 4s );
            }

        case State::ConnectedToCloud:
            setState( State::Completed );
            break;

        case State::Failed:
            // In failed state after reconnection to previous access point
            // we should send broadcast, thus we should wait until connect to
            // previous AP
            if( ClockType::now() - m_timePoint > 20s ) {
                FUNC_LOGW( "Can't reconnect to cloud from previous access point" );
                constexpr bool isRunStartupService{ false };
                restorePrevConnection( m_prevConf, isRunStartupService );
                m_timePoint = ClockType::now();
                return ClbkRet::Continue;
            }
            else {
                return setDelay( 4s );
            }

        default:
            break;
        }

        return ClbkRet::Completed;
    };

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const cloud_client::responces::ConnectedToCloud& command ) override {
        if( State::WaitingConnectionToCloud == m_state ) {
            m_state = State::ConnectedToCloud;
            sendHubNetworkChangedSuccessBroadcast();
        }
        else if( State::Failed == m_state ) {
            m_state = State::ConnectedToCloud;
            sendHubNetworkChangedFailedBroadcast( m_errorCode, m_errorDescr, m_ssid, m_passw, m_bssid );
        }
    }

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const cloud_client::responces::DisconnectedFromCloud& command ) override {
        if( State::WaitingNmaClientsShutdown == m_state ) {
            m_state = State::JoiningToNewAccessPoint;
            setDelay( 2s );
        }
    }

    void setState( const State newState ) {
        m_state = newState;
        m_timePoint = ClockType::now();
    }

private:
    State m_state{ State::Init };
    std::string m_ssid, m_passw;
    std::optional<network::wifi_srv2::BssidType> m_bssid;
    network::wifi_srv2::StaConfig m_prevConf;

    ClockType::time_point m_timePoint;
    bool m_isSomeClientShutdowned{ false };

    int m_errorCode{ errors::NotError };
    std::string m_errorDescr;
};

}  // namespace

WebSocketHandler::ResponceType WebSocketHandler::onHubNetworkWifiTryConnect( const gason::JsonValue& json,
                                                                             common::JsonStreamGenerator& jsGen ) {
    FUNC_LOGI( "WebSocketHandler::onHubNetworkWifiTryConnect" );

    if( !jsGen.writeEmptyObject() || !jsGen.writeItemDelimiter() ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    auto network = json( tokens::Params )( tokens::Network );
    if( !network ) {
        return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
    }

    std::string ssid, passw;
    std::optional<network::wifi_srv2::BssidType> bssid;
    {
        const auto ssidCStr = network( tokens::Ssid ).toString();
        if( !ssidCStr || 0 == std::strlen( ssidCStr ) ) {
            return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
        }
        ssid = ssidCStr;
        if( ssid.empty() ) {
            return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
        }

        const auto passwCStr = network( tokens::Key ).toString();
        if( passwCStr ) {
            passw = passwCStr;
        }

        const auto bssidCStr = network( tokens::Bssid ).toString();
        if( bssidCStr ) {
            bssid = network::wifi_srv2::BssidType::fromString( bssidCStr );
        }
    }

    auto prevConf = network::wifi_srv2::getStaConfig();
    if( !prevConf ) {
        FUNC_LOGE( "Can't get current STA config" );
        return std::make_tuple( errors::InternalError, errors::data::InternalOperationError );
    }

    auto instance = std::make_unique<TryConnectCommandExecutor>(
        std::move( ssid ), std::move( passw ), std::move( bssid ), std::move( *prevConf ) );
    if( !instance || !instance->initialize() ) {
        FUNC_LOGE( "Can't create TryConnectCommandExecutor instance" );
        return std::make_tuple( errors::InternalError, errors::data::InternalOperationError );
    }

    instance->setDelay( 2s );  // Delay to send command responce
    core::upd_srv::add( std::move( instance ), "TryConnectCommandExecutor" );

    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace nma

#endif  // not CONFIG_UART_API
