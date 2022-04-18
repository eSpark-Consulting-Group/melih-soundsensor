#include "WifiHttpServer.h"
#include "CommonResources.h"
#include "ISocket.h"
#include "WifiResources.h"
#include "HTTPParser.h"
#include "wifi_service2/Common.h"
#include "JsonStreamGenerator.h"
#include "TCPServer2.h"
#include "Thread.h"
#include "gason.hpp"
#include "syscore/commands/Shutdown.h"

#include <string>
#include <tuple>
#include <chrono>

#include "DebugUtils.h"

namespace cloud_client {
extern bool isConnectedToCloud();
}  // namespace cloud_client

namespace core {
extern const char* serial();
}  // namespace core

namespace network {
namespace wifi_srv2 {

namespace {

using namespace common::resources;
using namespace std::chrono_literals;

constexpr auto DEFAULT_LISTENING_PORT = 54'540;

enum class HttpRequestExecutionResult {
    Success,
    SetupComplete,
    ParseError,
    InternalError,
    UnknownMethod,
    ResourceNotFound,
    ServiceUnavailable,
    Last
};

class WiFiHttpServer : public network::HTTPParser {
    enum class Request { Undefined, Connect, Restart, ScanNetworks, GetStatus, Close };

public:
    using UPtr = std::unique_ptr<WiFiHttpServer>;

    WiFiHttpServer( ISocket::UPtr&& sock )
    : m_sock( std::move( sock ) ) {}

    bool update();

    inline auto isCloseServer() const {
        return m_isCloseServer;
    }

private:
    void processFinishedMessage( const char* buffer, const std::size_t buffSize ) override;
    void processUrlHeader( const char* url, const std::size_t buffSize ) override;

    char* getBuffer() override {
        return m_answerDataBuffer.data();
    }

    std::size_t getMaxBufferSize() override {
        return m_answerDataBuffer.size();
    }

private:
    void onRestartHubOverWifi( const char* buffer, const std::size_t buffSize );
    void onScanNetworksRequest( const char* buffer, const std::size_t buffSize );
    void onGetStatusRequest( const char* buffer, const std::size_t buffSize );
    void onConnectRequest( const char* buffer, const std::size_t buffSize );
    void onCloseRequest( const char* buffer, const std::size_t buffSize );

    int makeHttpResponce500InternalError( char* responceBuffer, std::size_t bufferSize );
    int makeHttpResponce404NotFound( char* responceBuffer, std::size_t bufferSize );
    int makeHttpResponce400BadRequest( char* responceBuffer, std::size_t bufferSize );
    int makeHeader200Ok( char* responceBuffer, std::size_t bufferSize );

    std::tuple<int, std::vector<char>> makeJsonApList( WifiApInfoSetType&& accessPoints );

    void processRequestExecutionResult( const HttpRequestExecutionResult requestResult,
                                        char* responceBuffer,
                                        const std::size_t bufferSize );

    bool parseConnectionRequest( const char* request, std::string& ssid, std::string& password );

private:
    ISocket::UPtr m_sock;

    Request m_request = Request::Undefined;
    std::array<char, 1024> m_recivedDataBuffer;
    std::array<char, 4096> m_answerDataBuffer;

    bool m_isRequestCompleted{ false };
    bool m_isCloseServer{ false };
};  // class WiFiHttpServer

bool WiFiHttpServer::update() {
    if( m_isRequestCompleted ) {
        return false;
    }

    auto retCode = m_sock->receive( m_recivedDataBuffer.data(), m_recivedDataBuffer.size() );
    if( 0 > retCode ) {
        return false;
    }
    m_recivedDataBuffer[retCode] = '\0';
    FUNC_LOGI( "Recived msg(size=%d): %s", retCode, m_recivedDataBuffer.data() );

    if( parse( m_recivedDataBuffer.data(), retCode ) ) {
        FUNC_LOGI( "Message parsed" );
    }

    return true;
}

void WiFiHttpServer::processFinishedMessage( const char* buffer, const std::size_t bufferSize ) {
    switch( m_request ) {
    case Request::Restart:
        onRestartHubOverWifi( buffer, bufferSize );
        break;
    case Request::ScanNetworks:
        onScanNetworksRequest( buffer, bufferSize );
        break;

    case Request::GetStatus:
        onGetStatusRequest( buffer, bufferSize );
        break;

    case Request::Connect:
        onConnectRequest( buffer, bufferSize );
        break;

    case Request::Close:
        onCloseRequest( buffer, bufferSize );
        break;

    default:
        break;
    }

    m_request = Request::Undefined;
}

void WiFiHttpServer::onRestartHubOverWifi( const char* buffer, const std::size_t bufferSize ) {
    FUNC_LOGI( "WiFiHttpServer::onRestartHubOverWifi" );

    auto answerBuffer = getBuffer();
    const auto answerBufferSize = getMaxBufferSize();

    const bool isSuccess = bus::sendFromNowhere<core::commands::Shutdown>( core::commands::BUS_NAME );
    if( isSuccess ) {
        processRequestExecutionResult( HttpRequestExecutionResult::SetupComplete, answerBuffer, answerBufferSize );
    }
    else {
        processRequestExecutionResult( HttpRequestExecutionResult::InternalError, answerBuffer, answerBufferSize );
    }
}

void WiFiHttpServer::onScanNetworksRequest( const char* buffer, const std::size_t bufferSize ) {
    FUNC_LOGI( "WiFiHttpServer::processScanNetworksRequest" );

    auto answerBuffer = getBuffer();
    const auto answerBufferSize = getMaxBufferSize();

    auto accessPoints = getAvailableAccessPointsAsync();
    if( !accessPoints.valid() ) {
        FUNC_LOGE( "Wi-Fi scan failed" );
        processRequestExecutionResult( HttpRequestExecutionResult::InternalError, answerBuffer, answerBufferSize );
        return;
    }
    const auto statusOfResult = accessPoints.wait_for( std::chrono::minutes( 5 ) );
    if( statusOfResult != std::future_status::ready ) {
        FUNC_LOGE( "Wi-Fi scan failed" );
        processRequestExecutionResult( HttpRequestExecutionResult::InternalError, answerBuffer, answerBufferSize );
        return;
    }

    auto [retCode, jsonApData] = makeJsonApList( accessPoints.get() );
    if( retCode < 0 ) {
        FUNC_LOGE( "Creating json part of answer failed" );
        processRequestExecutionResult( HttpRequestExecutionResult::InternalError, answerBuffer, answerBufferSize );
        return;
    }

    auto responseSize = makeHeader200Ok( answerBuffer, answerBufferSize );
    if( responseSize == -1 ) {
        FUNC_LOGE( "Creating HTTP 200 OK header failed" );
        processRequestExecutionResult( HttpRequestExecutionResult::InternalError, answerBuffer, answerBufferSize );
        return;
    }

    strncat(
        answerBuffer, "Content-Type: application/json\r\nContent-Length: ", answerBufferSize - strlen( answerBuffer ) );
    // Below we should use (retCode - 1) because method makeJsonApList push '\0' char in stream as C end of string
    // but strncat( answerBuffer, jsonApData.data() ...) doesn't add it this char into responce
    strncat( answerBuffer, std::to_string( retCode - 1 ).c_str(), answerBufferSize - strlen( answerBuffer ) );
    strncat( answerBuffer, "\r\n\r\n", answerBufferSize - strlen( answerBuffer ) );
    strncat( answerBuffer, jsonApData.data(), answerBufferSize - strlen( answerBuffer ) );

    processRequestExecutionResult( HttpRequestExecutionResult::Success, answerBuffer, answerBufferSize );
}

void WiFiHttpServer::onGetStatusRequest( const char* buffer, const std::size_t bufferSize ) {
    FUNC_LOGI( "WiFiHttpServer::onGetStatusRequest" );

    auto answerBuffer = getBuffer();
    const auto answerBufferSize = getMaxBufferSize();

    if( 0 > makeHeader200Ok( answerBuffer, answerBufferSize ) ) {
        FUNC_LOGE( "Creating HTTP 200 OK header failed" );
        processRequestExecutionResult( HttpRequestExecutionResult::InternalError, answerBuffer, answerBufferSize );
        return;
    }

    constexpr auto maxJsonPartSize = 512;
    std::vector<char> jsonStateBuffer( maxJsonPartSize );

    if( isConnectedToAcessPoint() ) {
        auto mac = MacType::get();
        auto info = network::wifi_srv2::getStaIpInfo();
        if( mac && info ) {
            snprintf( jsonStateBuffer.data(),
                      maxJsonPartSize,
                      jsonAnswerSuccessStatusPattern,
                      core::serial(),
                      info->ipToStr().c_str(),
                      mac->toHexString().c_str() );
        }
        else {
            FUNC_LOGE( "Can't get ip or mac" );
            processRequestExecutionResult( HttpRequestExecutionResult::InternalError, answerBuffer, answerBufferSize );
            return;
        }

        strncat( answerBuffer,
                 "Content-Type: application/json\r\nContent-Length: ",
                 answerBufferSize - strlen( answerBuffer ) );

        auto contentLength = std::to_string( static_cast<int>( strlen( jsonStateBuffer.data() ) ) );
        strncat( answerBuffer, contentLength.c_str(), answerBufferSize - strlen( answerBuffer ) );
        strncat( answerBuffer, "\r\n\r\n", answerBufferSize - strlen( answerBuffer ) );
        strncat( answerBuffer, jsonStateBuffer.data(), answerBufferSize - strlen( answerBuffer ) );
        processRequestExecutionResult( HttpRequestExecutionResult::Success, answerBuffer, answerBufferSize );
        return;
    }
    else {
        snprintf( jsonStateBuffer.data(),
                  maxJsonPartSize,
                  jsonStatusErrorAnswerPattern,
                  "error",
                  std::to_string( int( JoinResult::Undefined ) ).c_str(),
                  "not connected" );

        strncat( answerBuffer,
                 "Content-Type: application/json\r\nContent-Length: ",
                 answerBufferSize - strlen( answerBuffer ) );

        auto contentLength = std::to_string( static_cast<int>( strlen( jsonStateBuffer.data() ) ) );
        strncat( answerBuffer, contentLength.c_str(), answerBufferSize - strlen( answerBuffer ) );
        strncat( answerBuffer, "\r\n\r\n", answerBufferSize - strlen( answerBuffer ) );
        strncat( answerBuffer, jsonStateBuffer.data(), answerBufferSize - strlen( answerBuffer ) );

        processRequestExecutionResult( HttpRequestExecutionResult::Success, answerBuffer, answerBufferSize );
        return;
    }
}

bool WiFiHttpServer::parseConnectionRequest( const char* request, std::string& ssid, std::string& password ) {
    FUNC_LOGI( "WiFiHttpServer::parseConnectionRequest: request=%s", request );

    std::unique_ptr<gason::JsonAllocator> jsonAlloc = std::make_unique<gason::JsonAllocator>();
    if( !jsonAlloc ) {
        FUNC_LOGW( "Can't create json alloc" );
        return false;
    }

    gason::JsonValue json;
    std::string ps = request;
    if( gason::JSON_PARSE_OK != gason::jsonParse( const_cast<char*>( ps.c_str() ), json, *jsonAlloc ) ) {
        FUNC_LOGW( "Can't parse json data" );
        return false;
    }

    auto wssid = json( "ssid" ).toString();
    if( !wssid ) {
        FUNC_LOGW( "Wifi \"ssid\" not found" );
        return false;
    }
    ssid = wssid;

    auto wpass = json( "passkey" ).toString();
    if( !wpass ) {
        FUNC_LOGW( "Wifi \"passkey\" not found" );
        return false;
    }
    password = wpass;

    return true;
}

void WiFiHttpServer::onConnectRequest( const char* buffer, const std::size_t bufferSize ) {
    FUNC_LOGI( "WiFiHttpServer::onConnectRequest: request=%s", buffer );

    auto answerBuffer = getBuffer();
    const auto answerBufferSize = getMaxBufferSize();

    std::string staSsid, staPassword;
    if( !parseConnectionRequest( buffer, staSsid, staPassword ) ) {
        FUNC_LOGE( "Parsing connection request failed" );
        processRequestExecutionResult( HttpRequestExecutionResult::ParseError, answerBuffer, answerBufferSize );
        return;
    }

    if( !joinToAccessPoint( staSsid, staPassword ) ) {
        FUNC_LOGE( "Can't join to access point" );
        processRequestExecutionResult( HttpRequestExecutionResult::ServiceUnavailable, answerBuffer, answerBufferSize );
        return;
    }

    unsigned int probesCount = 5;
    do {
        if( 0 == probesCount ) {
            FUNC_LOGW( "Can't connect to access point" );
            wipeStaConfig();
            processRequestExecutionResult(
                HttpRequestExecutionResult::ResourceNotFound, answerBuffer, answerBufferSize );
            return;
        }
        probesCount -= 1;
        core::this_thread::sleep_for( 1500ms );
    } while( !isConnectedToAcessPoint() );

    auto answerCurrentSize = makeHeader200Ok( answerBuffer, answerBufferSize );
    strncat( answerBuffer, "Content-Type: application/json\r\nContent-Length: ", answerBufferSize - answerCurrentSize );
    strncat( answerBuffer,
             std::to_string( static_cast<int>( strlen( jsonConnectSuccessAnswer ) ) ).c_str(),
             answerBufferSize - strlen( answerBuffer ) );
    strncat( answerBuffer, "\r\n\r\n", answerBufferSize - strlen( answerBuffer ) );
    strncat( answerBuffer, jsonConnectSuccessAnswer, answerBufferSize - strlen( answerBuffer ) );
    processRequestExecutionResult( HttpRequestExecutionResult::Success, answerBuffer, answerBufferSize );
}

void WiFiHttpServer::onCloseRequest( const char* buffer, const std::size_t bufferSize ) {
    FUNC_LOGI( "WiFiHttpServer::processCloseRequest" );

    auto answerBuffer = getBuffer();
    auto answerBufferSize = getMaxBufferSize();

    int currentSize = makeHeader200Ok( answerBuffer, answerBufferSize );
    strncat( answerBuffer, "Connection: close\r\n\r\n\r\n", answerBufferSize - currentSize );
    processRequestExecutionResult( HttpRequestExecutionResult::SetupComplete, answerBuffer, answerBufferSize );
}

void WiFiHttpServer::processRequestExecutionResult( const HttpRequestExecutionResult requestResult,
                                                    char* buffer,
                                                    const std::size_t bufferSize ) {
    auto answerSize = std::strlen( buffer );
    switch( requestResult ) {
    case HttpRequestExecutionResult::InternalError:
        answerSize = makeHttpResponce500InternalError( buffer, bufferSize );
        break;

    case HttpRequestExecutionResult::ResourceNotFound:
    case HttpRequestExecutionResult::UnknownMethod:
    case HttpRequestExecutionResult::ParseError:
    case HttpRequestExecutionResult::ServiceUnavailable:
        answerSize = makeHttpResponce400BadRequest( buffer, bufferSize );
        break;

    case HttpRequestExecutionResult::Success:
    default:
        break;
    }

    buffer[answerSize] = '\0';
    if( m_sock->sendAll( buffer, answerSize ) ) {
        FUNC_LOGD( "Sended answer(size=%zu):\n%s", answerSize, buffer );
        if( requestResult == HttpRequestExecutionResult::SetupComplete ) {
            FUNC_LOGI( "Finished WifiHttpServer" );
            m_isCloseServer = true;
        }
        m_isRequestCompleted = true;
    }
    else {
        FUNC_LOGE( "Sended answer failed. Answer size = %zu", answerSize );
    }
}

void WiFiHttpServer::processUrlHeader( const char* url, const std::size_t buffSize ) {
    switch( getMethod() ) {
    case HttpMethod::POST: {
        if( strstr( url, "/wifi/network/connect" ) != nullptr ) {
            m_request = Request::Connect;
        }
        else if( strstr( url, "/restart" ) != nullptr ) {
            m_request = Request::Restart;
        }
        else if( strstr( url, "/wifi/network/close" ) ) {
            m_request = Request::Close;
        }
    } break;

    case HttpMethod::GET: {
        if( strstr( url, "/wifi/networks" ) != nullptr ) {
            m_request = Request::ScanNetworks;
        }
        else if( strstr( url, "/wifi/status" ) != nullptr ) {
            FUNC_LOGD( "/wifi/status" );
            m_request = Request::GetStatus;
        }
    } break;

    default:
        FUNC_LOGW( "Undefined http request!" );
        break;
    };
}

int WiFiHttpServer::makeHttpResponce500InternalError( char* buffer, const std::size_t bufferSize ) {
    int answerSize = snprintf( buffer, bufferSize, http500Answer, AP_HTTP_SERVER_NAME );
    return ( answerSize <= bufferSize ) ? answerSize : -1;
}

int WiFiHttpServer::makeHttpResponce404NotFound( char* buffer, const std::size_t bufferSize ) {
    int answerSize = snprintf( buffer, bufferSize, http404Answer, AP_HTTP_SERVER_NAME );
    return ( answerSize > 0 && answerSize < bufferSize ) ? answerSize : -1;
}

int WiFiHttpServer::makeHttpResponce400BadRequest( char* buffer, const std::size_t bufferSize ) {
    int answerSize = snprintf( buffer, bufferSize, http400Answer, AP_HTTP_SERVER_NAME );
    return ( answerSize > 0 && answerSize < bufferSize ) ? answerSize : -1;
}

int WiFiHttpServer::makeHeader200Ok( char* buffer, std::size_t bufferSize ) {
    auto answerSize = snprintf( buffer, bufferSize, http200Header, AP_HTTP_SERVER_NAME );
    return ( answerSize > 0 && answerSize < bufferSize ) ? answerSize : -1;
}

std::tuple<int, std::vector<char>> WiFiHttpServer::makeJsonApList( WifiApInfoSetType&& accessPoints ) {
    common::VectorWStream wstream;
    common::JsonStreamGenerator jsGen( wstream );

    bool is_success = jsGen.writeOpeningObject();
    is_success = is_success && jsGen.writeProperty( "status", "success" );

    is_success = is_success && jsGen.writePropertyKey( "networks" );
    is_success = is_success && jsGen.writeOpeningArray();  // [
    if( is_success ) {
        for( const auto& ap : accessPoints ) {
            is_success = is_success && jsGen.writeOpeningObject();  // {
            is_success = is_success && jsGen.writeProperty( "ssid", ap.ssidToString() );
            is_success = is_success && jsGen.writeProperty( "signalLevel", ap.getRssi().rssiToPercent() );
            is_success = is_success && jsGen.writeProperty( "open", ap.isOpen() );
            is_success = is_success && jsGen.writeClosingSymbol();  // }
        }
    }
    is_success = is_success && jsGen.writeClosingSymbol();  // ]
    is_success = is_success && jsGen.writeJsonEnd();

    if( !is_success ) {
        return std::make_tuple( -1, std::vector<char>{} );
    }

    // Because due to task https://jira.mios.com/browse/EATT-1693
    // json generator doesn't add '\0' in the end of blob
    wstream.write( '\0' );
    return std::make_tuple( wstream.dataSize(), std::move( wstream.buffer() ) );
}

}  // namespace

core::Thread runWifiHttpServerThread( std::atomic<HttpServerState>& httpServerState ) {
    auto clbk = [&httpServerState]() {
        httpServerState = HttpServerState::Started;

        network::TCPServer2 srv;
        std::list<WiFiHttpServer::UPtr> clients;

        if( !srv.listen( std::string(), DEFAULT_LISTENING_PORT ) ) {
            FUNC_LOGE( "Can't listen port=%d", DEFAULT_LISTENING_PORT );
            return;
        }

        httpServerState = HttpServerState::Run;
        while( HttpServerState::Run == httpServerState ) {
            {
                auto sock = srv.nextPendingConnection();
                bool isSuccess = sock && sock->setBlocking( true ) &&
                                 sock->setWriteTimeout( std::chrono::seconds( 5 ) ) &&
                                 sock->setReadTimeout( std::chrono::seconds( 5 ) );
                if( isSuccess ) {
                    auto client = std::make_unique<WiFiHttpServer>( std::move( sock ) );
                    clients.emplace_back( std::move( client ) );
                }
            }

            {
                const auto end = std::end( clients );
                auto itr = std::begin( clients );
                while( end != itr ) {
                    if( ( *itr )->update() ) {
                        itr = std::next( itr );
                    }
                    else {
                        const auto isServerMustClosed = ( *itr )->isCloseServer();
                        itr = clients.erase( itr );
                        if( isServerMustClosed ) {
                            goto exit;
                        }
                    }
                }
            }

            core::this_thread::yield();
        }

    exit:
        // Fall there if HttpServerState::Shutdowned == httpServerState
        httpServerState = HttpServerState::Inactive;
        stopAccessPoint();
    };  // end of clbk

    return core::Thread( "WifiHttpServer", 4 * 1024, clbk );
}

}  // namespace wifi_srv2
}  // namespace network