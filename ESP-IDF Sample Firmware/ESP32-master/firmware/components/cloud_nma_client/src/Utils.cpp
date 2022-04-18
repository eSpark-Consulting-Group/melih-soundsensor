#include "sdkconfig.h"

#include "cloud_nma_client/Utils.h"

#include "SSLSocket.h"
#include "Ezlo.h"
#include "IDInfo.h"
#include "CommonResources.h"
#include "HTTPClient.h"
#include "Thread.h"
#include "Utils.h"
#include "time_service/Utils.h"
#include "wifi_service2/Common.h"
#include "CloudNmaClientStartupService.h"

#include <mutex>
#include <atomic>

#include "DebugUtils.h"

namespace cloud_client {

using namespace std::chrono_literals;
namespace {
NmaInfo g_latestNma;
std::mutex g_latestNmaMutex;

std::string getNmaCoordinatorAddr() {
    constexpr char HTTP_PREFIX[] = "http://";
    constexpr std::uint16_t HTTP_PORT = 80u;

    constexpr char HTTPS_PREFIX[] = "https://";
    constexpr std::uint16_t HTTPS_PORT = 443u;

    std::string addr = core::IDInfo::get().cloudServerURL();

    if( ':' != addr.back() ) {
        return std::move( addr );
    }

    if( common::stringStartsWith( addr.c_str(), HTTP_PREFIX ) ) {
        addr += std::to_string( HTTP_PORT );
    }
    else if( common::stringStartsWith( addr.c_str(), HTTPS_PREFIX ) ) {
        addr += std::to_string( HTTPS_PORT );
    }
    else {
        addr += std::to_string( HTTPS_PORT );
    }

    return std::move( addr );
}

}  // namespace

constexpr static size_t ParserMaxBufferLength = 256 + 32;

constexpr char COORDINATOR_URL_TEMPLATE[] = "%s/getserver?serial=%s";
constexpr char COORDINATOR_SHORT_URL_TEMPLATE[] = "%s/getserver";
constexpr char COORDINATOR_URL_TEMPLATE_WITH_NMA[] = "%s/getserver?serial=%s&skip=%s:%d";
constexpr char COORDINATOR_SHORT_URL_TEMPLATE_WITH_NMA[] = "%s/getserver?skip=%s:%d";

constexpr char SecureWebSocketPrefix[] = "wss://";
constexpr uint16_t SecureWebSocketPort = 443;

constexpr char NotSecureWebSocketPrefix[] = "ws://";
constexpr uint16_t NotSecureWebSocketPort = 80;

bool isConnectionExist( const std::string& oldSrvName, const unsigned int oldSrvPort ) {
    auto threadTask = []( const std::string& oldSrvName, const unsigned int oldSrvPort ) {
        FUNC_LOGI( "isConnectionExist task created" );
        auto requestTimes = 1;
        auto [srvName, port, isSecured] = requestNewNmaAddress( oldSrvName, oldSrvPort, requestTimes );
        const bool isConnExist = !srvName.empty();
        if( isConnExist ) {
            std::lock_guard<decltype( g_latestNmaMutex )> guard( g_latestNmaMutex );
            g_latestNma = std::make_tuple( std::move( srvName ), port, isSecured );
        }
    };

    constexpr auto stackSize = 7 * 1024;
    core::Thread isConnThread(
        "isConnectionExistTask", stackSize, std::move( threadTask ), std::ref( oldSrvName ), oldSrvPort );

    isConnThread.join();

    const bool isConnExist = !std::get<0>( getLatestNma() ).empty();
    if( isConnExist ) {
        FUNC_LOGI( "Connection with server exist" );
        return true;
    }
    else {
        FUNC_LOGW( "Connection with server doesn't exist" );
        return false;
    }
}

NmaInfo requestNewNmaAddress( const std::string& oldNmaAddress,
                              const unsigned int oldPort,
                              const unsigned int requestAttempts ) {
#ifdef CONFIG_MBEDTLS_HAVE_TIME
    if( !time_service::waitingTimeDidEstablishedFor( 20s ) ) {
        bool isConnectionSecured = true;
        return std::make_tuple( std::string(), 0, isConnectionSecured );
    }
#endif  // CONFIG_MBEDTLS_HAVE_TIME
    network::SSLSocket sslSocket;
    core::IDInfo::get();
    sslSocket.setCertificates( core::IDInfo::get().CACertificate(),
                               core::IDInfo::get().TLSPrivateKey(),
                               core::IDInfo::get().TLSPublicCertificate() );

    network::HTTPClient httpClient( sslSocket );
    httpClient.setMaxRedirections( 3 );

    bool isConnectionSecured = true;
    const auto serial = core::serial();
    auto result = network::HTTPResult::OK;
    uint8_t gettingAddressAttempts = 0;
    char nmaParserBuffer[ParserMaxBufferLength];

    do {
        const auto nmaCoordAddr = getNmaCoordinatorAddr();
        FUNC_LOGD( "Attempt to get NMA server address from '%s' #%d", nmaCoordAddr.c_str(), ++gettingAddressAttempts );

        if( gettingAddressAttempts > 2 ) {
            if( !oldNmaAddress.empty() ) {
                snprintf( nmaParserBuffer,
                          sizeof( nmaParserBuffer ),
                          COORDINATOR_SHORT_URL_TEMPLATE_WITH_NMA,
                          nmaCoordAddr.c_str(),
                          oldNmaAddress.c_str(),
                          oldPort );
            }
            else {
                snprintf(
                    nmaParserBuffer, sizeof( nmaParserBuffer ), COORDINATOR_SHORT_URL_TEMPLATE, nmaCoordAddr.c_str() );
            }
        }
        else {
            if( !oldNmaAddress.empty() ) {
                snprintf( nmaParserBuffer,
                          sizeof( nmaParserBuffer ),
                          COORDINATOR_URL_TEMPLATE_WITH_NMA,
                          nmaCoordAddr.c_str(),
                          serial,
                          oldNmaAddress.c_str(),
                          oldPort );
            }
            else {
                snprintf( nmaParserBuffer,
                          sizeof( nmaParserBuffer ),
                          COORDINATOR_URL_TEMPLATE,
                          nmaCoordAddr.c_str(),
                          serial );
            }
        }

        FUNC_LOGD( "Request NMA server: %s", nmaParserBuffer );
        result = httpClient.get( nmaParserBuffer, nmaParserBuffer, sizeof( nmaParserBuffer ) );

        if( result == network::HTTPResult::OK ) {
            uint8_t serverUrlStartIdx = 0, serverUrlEndIdx = 0, portStartIdx = 0;
            uint16_t serverPort = 0;
            isConnectionSecured = true;

            FUNC_LOGD( "Arrived from coordinator: %s", nmaParserBuffer );

            if( strstr( nmaParserBuffer, SecureWebSocketPrefix ) == nmaParserBuffer ) {
                serverUrlStartIdx = sizeof( SecureWebSocketPrefix ) - 1;
                serverPort = SecureWebSocketPort;
                isConnectionSecured = true;
            }
            else if( strstr( nmaParserBuffer, NotSecureWebSocketPrefix ) == nmaParserBuffer ) {
                serverUrlStartIdx = sizeof( NotSecureWebSocketPrefix ) - 1;
                serverPort = NotSecureWebSocketPort;
                isConnectionSecured = false;
            }

            if( serverUrlStartIdx == 0 ) {
                FUNC_LOGW( "Unsupported protocol: %s", nmaParserBuffer );
            }
            else {
                serverUrlEndIdx = serverUrlStartIdx;

                for( auto* searchPosition = nmaParserBuffer + serverUrlStartIdx; *searchPosition;
                     ++searchPosition, ++serverUrlEndIdx ) {
                    switch( *searchPosition ) {
                    case ':':
                        portStartIdx = serverUrlEndIdx + 1;
                        searchPosition = nullptr;
                        break;

                    case '/':
                    case '\\':
                    case '\0':
                    case '\n':
                    case '\r':
                    case '\t':
                        searchPosition = nullptr;
                        break;

                    default:
                        break;
                    };

                    if( searchPosition == nullptr ) {
                        break;
                    }
                }

                if( portStartIdx != 0 ) {
                    sscanf( nmaParserBuffer + portStartIdx, "%" SCNu16, &serverPort );
                }

                if( serverUrlEndIdx != 0 ) {
                    nmaParserBuffer[serverUrlEndIdx] = '\0';
                }

                FUNC_LOGI( "Got NMA server: %s:%" PRIu16, nmaParserBuffer + serverUrlStartIdx, serverPort );

                return std::make_tuple(
                    std::string( nmaParserBuffer + serverUrlStartIdx ), serverPort, isConnectionSecured );
            }
        }

        if( gettingAddressAttempts >= requestAttempts ) {
            break;
        }

        FUNC_LOGW( "Cannot get NMA server: %s", network::HTTPClient::GetErrorMessage( result ) );
        core::this_thread::sleep_for( std::chrono::seconds( 5 ) );

        if( !network::wifi_srv2::isConnectedToAcessPoint() ) {
            break;
        }

    } while( result != network::HTTPResult::OK );

    return std::make_tuple( std::string(), 0, isConnectionSecured );
}

std::tuple<std::string, unsigned int, bool> getLatestNma() {
    std::lock_guard<decltype( g_latestNmaMutex )> guard( g_latestNmaMutex );
    return g_latestNma;
}

#ifndef CONFIG_UART_API
bool createAndRunStartupService( const std::chrono::seconds& delay,
                                 std::shared_ptr<core::EspLogger> logger,
                                 ::gateways::IGatewayUserDataStorage& stor ) {
    return CloudNmaClientStartupService::createAndRun( delay, logger, stor );
}
#endif  // not CONFIG_UART_API
}  // namespace cloud_client
