#include "sdkconfig.h"

#ifndef CONFIG_UART_API

#include "CloudNmaClient.h"

#include <NmaCommand.h>
#include "NmaJsonHandler.h"
#include "CloudNmaClientStartupService.h"
#include "wifi_service2/Common.h"
#include "wifi_service2/WifiCommands.h"
#include <Storage.h>
#include <BinaryFileStorageFactory.h>
#include <StorageSection.h>
#include <ObjectSerializer.h>
#include "INmaJsonHandler.h"

#include <CloudDevicesStorage.h>
#include "ControllerStorageTokenInfo.h"

#include "JsonSocket.h"
#include <HTTPData/HTTPText.h>
#include <CommonResources.h>
#include <JsonStreamGenerator.h>
#include <CommonResources.h>
#include <GatewayUtils.h>

#include <FirmwareVersion.h>
#include <Utils.h>
#include <HardUtils.h>
#include <Ezlo.h>
#include <IDInfo.h>

#include "BusCommandNmaClientClbk.h"
#include <BusCommandSetHandler.h>
#include <BusCommandHandleEventUpdateProductName.hpp>
#include <BusCommandFeatureFlag.hpp>
#include <gateways/commands/BusCommandProvisioningListEntryAdded.hpp>
#include <gateways/commands/BusCommandProvisioningListEntryUpdated.hpp>
#include <gateways/commands/BusCommandProvisioningListEntryRemoved.hpp>

#include <other_commands/BusCommandCreateChipBackup.h>
#include <other_commands/BusCommandRestoreChipBackup.h>

#include "cloud_client/responces/ConnectedToCloud.h"
#include "cloud_client/responces/DisconnectedFromCloud.h"
#include "cloud_client/commands/HubProvisionChanged.h"
#include "cloud_client/commands/ProvisionSyncingCompleted.h"

#include <values/BoolValue.h>
#include <IValueCast.h>

#include "TCPSocket.h"
#include "SSLSocket.h"

#include <memory>
#include <chrono>
#include <thread>
#include <ctime>

#include <NmaBroadcastsSender.h>
#include <Cache.h>
#include <IDInfoManager.h>

#include <RemoteLogger.h>
#include <MemoryMappedFlash.h>
#include <FlashUtils.h>
#include <mbedtls/base64.h>
#include <DeviceInfo.h>

#include <values/TableValue.h>
#include <values/StringValue.h>
#include "HouseModesManager.h"
#include "FirmwareUpdater.h"
#include "cloud_nma_client/Utils.h"
#include "Thread.h"
#include "BackupCloudStorage.h"

#define ENABLE_NMA_LOG
#ifdef ENABLE_NMA_LOG

#include "DebugUtils.h"

#define NMA_CLIENT_TAG "[CloudNmaClient"
#define NMA_LOGE( _FMT, ... ) ESP_LOGE( NMA_CLIENT_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )
#define NMA_LOGW( _FMT, ... ) ESP_LOGW( NMA_CLIENT_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )
#define NMA_LOGI( _FMT, ... ) ESP_LOGI( NMA_CLIENT_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )
#define NMA_LOGD( _FMT, ... ) ESP_LOGD( NMA_CLIENT_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )

#else

#define NMA_LOGE( ... ) \
    do {                \
    } while( 0 )
#define NMA_LOGW( ... ) \
    do {                \
    } while( 0 )
#define NMA_LOGI( ... ) \
    do {                \
    } while( 0 )
#define NMA_LOGD( ... ) \
    do {                \
    } while( 0 )

#endif

namespace cloud_client {
namespace {

using namespace common::resources;
using namespace std::chrono_literals;

constexpr std::chrono::hours ProvisionDataUpdatePeriod( 24 );
constexpr std::chrono::seconds ScanAvailableAccessPointsPeriod( 15 );

void checkProvisioningThreadTask( const std::string oldNmaServer, const unsigned int oldNmaPort ) {
    const auto syncRes = core::provision::syncProvisionData();
    if( core::provision::SyncRes::Failed == syncRes || core::provision::SyncRes::NoChanges == syncRes ) {
        bus::sendFromNowhere<commands::ProvisionSyncingCompleted>( commands::BUS_NAME );
        return;
    }

    bus::sendFromNowhere<bus::command::HandleEventUpdateProductName>(
        core::commands::BUS_NAME, std::string{}, std::string{}, 0 );

    constexpr int connectionAttempts = 5;
    auto [srvName, srvPort, isConnSecured] = requestNewNmaAddress( oldNmaServer, oldNmaPort, connectionAttempts );
    if( srvName.empty() ) {
        bus::sendFromNowhere<commands::ProvisionSyncingCompleted>( commands::BUS_NAME );
    }
    else {
        bus::sendFromNowhere<commands::HubProvisionChanged>( commands::BUS_NAME, srvName, srvPort, isConnSecured );
    }
};

}  // namespace

unsigned int CloudNmaClient::m_nmaPort = CloudNmaClient::InvalidNmaPort;
bool CloudNmaClient::m_isConnectionSecured = true;
bool CloudNmaClient::m_isNewServerRequest = true;
int CloudNmaClient::m_syncProvisionDataAttempCount = 1;
std::string CloudNmaClient::m_nmaServer;

bool CloudNmaClient::startCheckProvisionTask() {
    core::Thread thread( "CheckingProvisionTask", 8 * 1024, checkProvisioningThreadTask, nmaServer(), nmaPort() );
    if( thread ) {
        thread.detach();
        return true;
    }

    FUNC_LOGW( "Can't start CheckingProvisionTask" );
    return false;
}

CloudNmaClient::CloudNmaClient( std::shared_ptr<::core::EspLogger> espLogger,
                                ::gateways::IGatewayUserDataStorage& gatewayUserDataStorage )
: nma::NmaBase( createSocket(), espLogger, gatewayUserDataStorage ) {
    NMA_LOGI( "Constructor(...)" );
}

CloudNmaClient::~CloudNmaClient() {
    const auto utcTimeStr = getUTCTimeStr();
    NMA_LOGI( "Destructor() at %s", utcTimeStr.data() );
    if( m_isRestartStartupService ) {
        NMA_LOGI( "Create and run CloudNmaClientStartupService instance" );
        CloudNmaClientStartupService::createAndRun(
            m_isNewServerRequest ? 1s : 5s, m_espLogger, m_gatewayUserDataStorage );
    }
}

void CloudNmaClient::onConnected() {
    m_isConnected = true;
    joinToBus();

    m_checkProvisionTimepoint = ClockType::now() + ProvisionDataUpdatePeriod;
}

void CloudNmaClient::onConnLost() {
    const auto utcTimeStr = getUTCTimeStr();
    FUNC_LOGW( "Close connection to NMA at %s", utcTimeStr.data() );
    m_isConnected = false;
    leaveBus();
    m_socket->close();
    jsonHandler().reset();

    send<responces::DisconnectedFromCloud>( responces::BUS_NAME );
}

bool CloudNmaClient::initializeImpl() {
    if( !NmaBase::initializeImpl() || !bus::Bus::get().join( commands::BUS_NAME, *this ) ) {
        return false;
    }

    setJsonHandler( std::make_unique<NmaJsonHandler>( *this, m_espLogger, m_gatewayUserDataStorage ) );

    return true;
}

network::ISocket::UPtr CloudNmaClient::createSocket() {
    if( m_isConnectionSecured ) {
        auto securedSock = std::make_unique<network::SSLSocket>();
        securedSock->setCertificates( core::IDInfo::get().CACertificate(),
                                      core::IDInfo::get().TLSPrivateKey(),
                                      core::IDInfo::get().TLSPublicCertificate() );
        return std::make_unique<network::JsonSocket>( std::move( securedSock ) );
    }
    else {
        auto tcpSock = std::make_unique<network::TCPSocket>();
        return std::make_unique<network::JsonSocket>( std::move( tcpSock ) );
    }
}

bool CloudNmaClient::joinToBus() {
    return NmaBase::joinToBus() && bus::Bus::get().join( bus::command::FEATURE_FLAG_BUS_CLIENT_NAME, *this ) &&
           bus::Bus::get().join( network::wifi_srv2::responce::BUS_NAME, *this );
}

bool CloudNmaClient::connectToNma( const std::string& srvName, const unsigned int srvPort ) {
    NMA_LOGD( "Attempt to connect to '%s':%d", srvName.c_str(), srvPort );

    if( !m_socket->connectToHost( srvName, srvPort ) ) {
        NMA_LOGW( "Can't connect to the NMA server" );
        return false;
    }

    const auto utcTimeStr = getUTCTimeStr();
    NMA_LOGI( "Connection to the NMA server is established at %s", utcTimeStr.data() );
    return true;
}

bool CloudNmaClient::sendRegisterHubRequest() {
    auto stream = writeStream();
    common::JsonStreamGenerator jsGen( *stream );

    bool result = jsGen.writeOpeningObject();
    {
        if( result ) {
            std::array<char, 32> idBuffer;
            snprintf( idBuffer.data(),
                      idBuffer.size(),
                      "AT32-%" PRIX32 "-%" PRIX32,
                      hard::common::generateUniqueId(),
                      hard::common::generateUniqueId() );
            result = jsGen.writeProperty( tokens::Id, idBuffer.data() );
        }

        result = result && jsGen.writeProperty( tokens::Method, "register" );
        result = result && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
        result = result && jsGen.writePropertyKey( tokens::Params );
        result = result && jsGen.writeOpeningObject();
        result = result && jsGen.writeProperty( "serial", core::serial() );
        result = result && jsGen.writeProperty( "controller_uuid", core::Ezlo::uuid() );
        result = result && jsGen.writeProperty( "firmware", FIRMWARE_VERSION );
        result = result && jsGen.writeProperty( tokens::Media, tokens::Radio );

        if( result ) {
            auto ezlo = core::Ezlo::get();
            if( ezlo ) {
                result = jsGen.writeProperty( tokens::TimeOffset, ezlo->getTimeOffset() ) &&
                         jsGen.writeProperty( tokens::HubType, ezlo->hubType() );
            }
        }

        result = result && jsGen.writeProperty( tokens::MaxFrameSize, CONFIG_MBEDTLS_SSL_IN_CONTENT_LEN );
        auto mac = network::wifi_srv2::MacType::get();
        result = result && mac && jsGen.writeProperty( tokens::MacAddress, mac->toHexString() );
    }
    result = result && jsGen.writeClosingSymbol();

    return result && jsGen.writeJsonEnd();
}

void CloudNmaClient::closeConnection() {
    NMA_LOGW( "Close cloud nma connection" );
    onConnLost();
}

void CloudNmaClient::onRegistered() {
    const auto utcTimeStr = getUTCTimeStr();
    NMA_LOGI( "Successfully registered on cloud at %s", utcTimeStr.data() );

    m_isFirmwareUpdate = sendFirmwareUpdateRequest();
    m_fwUpdateTimepoint = ClockType::now() + FwUpdatePeriod;

    send<responces::ConnectedToCloud>( responces::BUS_NAME );

#ifdef CONFIG_CLOUD_DEVICES
    requestAbstractList();
#endif  // CONFIG_CLOUD_DEVICES
}

bool CloudNmaClient::tryConnectToNma() {
    NMA_LOGI( "tryConnectToNma" );

    if( !connectToNma( nmaServer(), nmaPort() ) ) {
        return false;
    }

    onConnected();

    if( sendRegisterHubRequest() ) {
        NMA_LOGI( "Hub registered request sended to cloud successfully" );
        return true;
    }

    NMA_LOGE( "Can't send register hub request to cloud!" );
    return false;
}

bool CloudNmaClient::getNewNmaAddress() {
    NMA_LOGI( "getNewNmaAddress" );

    constexpr auto connectionAttempts = 5;
    auto [srvName, srvPort, isConnSecured] = requestNewNmaAddress( nmaServer(), nmaPort(), connectionAttempts );
    if( srvName.empty() ) {
        return false;
    }

    setServerInfo( srvName, srvPort, isConnSecured );
    return true;
}

void CloudNmaClient::updateProvisionDataRequest() {
    NMA_LOGI( "call updateProvisionDataRequest()" );
    m_isNewServerRequest = true;
    m_isNeedReconnection = true;
}

bool CloudNmaClient::processNewServerRequest() {
    const auto syncRes = core::provision::syncProvisionData();
    if( core::provision::SyncRes::Failed == syncRes ) {
        m_syncProvisionDataAttempCount++;
        return m_syncProvisionDataAttempCount > SyncProvisioningItemsMaxNumber;
    }
    else if( core::provision::SyncRes::Updated == syncRes ) {
        NMA_LOGD( "Provision data was updated" );
        m_syncProvisionDataAttempCount = 0;
        bus::sendFromNowhere<bus::command::HandleEventUpdateProductName>(
            core::commands::BUS_NAME, std::string{}, std::string{}, 0 );
    }
    else {
        m_syncProvisionDataAttempCount = 0;
        NMA_LOGD( "Provision data up to date" );
    }

    return getNewNmaAddress();
}

#ifdef CONFIG_CLOUD_DEVICES
bool CloudNmaClient::requestAbstractList() {
    char version;
    if( !gateways::cloud::DevicesStorage::get().readVersion( version ) ) {
        return false;
    }

    auto ws = writeStream();
    if( !ws ) {
        return false;
    }
    common::JsonStreamGenerator gen( *ws );

    return gen.writeProperty( tokens::Id, core::Ezlo::uuid() ) &&
           gen.writeProperty( tokens::Method, common::resources::commands::CloudControllerAbstractsList ) &&
           gen.writeProperty( tokens::MsgId, m_handler->getMsgId() ) && gen.writeProperty( tokens::Api, "1" ) &&
           gen.writePropertyKey( tokens::Params ) && gen.writeOpeningObject() &&
           gen.writeProperty( tokens::ExtensionVersion, version ) && gen.writeClosingSymbol() && gen.writeJsonEnd();
}

bool CloudNmaClient::requestStorageControllerToken() {
    if( m_isStorageControllerTokenRequest ) {
        return true;
    }

    auto ws = writeStream();
    if( !ws ) {
        return false;
    }
    common::JsonStreamGenerator gen( *ws );

    m_isStorageControllerTokenRequest =
        gen.writeProperty( tokens::Id, core::Ezlo::uuid() ) &&
        gen.writeProperty( tokens::Method, common::resources::commands::CloudStorageControllerToken ) &&
        gen.writeProperty( tokens::MsgId, m_handler->getMsgId() ) && gen.writeProperty( tokens::Api, "1" ) &&
        gen.writePropertyKey( tokens::Params ) && gen.writeOpeningObject() && gen.writeClosingSymbol() &&
        gen.writeJsonEnd();

    return m_isStorageControllerTokenRequest;
}
#endif  // CONFIG_CLOUD_DEVICES

void CloudNmaClient::getStorageFileList( const char* storageType ) {
    auto ws = writeStream();
    common::JsonStreamGenerator gen( *ws );
    gen.writeProperty( tokens::Method, "cloud.storage_list" );
    gen.writeProperty( "api", "1" );
    gen.writePropertyKey( "params" );
    gen.writeOpeningObject();
    gen.writeProperty( "type", storageType );
    gen.writeJsonEnd();
}

bool CloudNmaClient::performConnect() {
    if( m_isNewServerRequest ) {
        if( !processNewServerRequest() ) {
            return false;
        }

        m_isNewServerRequest = false;
    }
    else if( !isValidServer() && !getNewNmaAddress() ) {
        return false;
    }

    return tryConnectToNma();
}

CloudNmaClient::ClbkRet CloudNmaClient::updateComponent( UpdContext& ctx ) {
    if( isShutdownStart() ) {
        return ClbkRet::Continue;
    }

    if( m_isNeedReconnection || ClbkRet::Completed == nma::NmaBase::updateComponent( ctx ) ) {
        closeConnection();
        return ClbkRet::Completed;
    }

    if( m_isFirmwareUpdate || m_isCheckProvisioning
#ifdef CONFIG_CLOUD_DEVICES
        || m_isStorageControllerTokenRequest
#endif  // CONFIG_CLOUD_DEVICES
    ) {
        return ClbkRet::Continue;
    }

    if( 0 < m_fwUpdateTimepoint.time_since_epoch().count() && ClockType::now() > m_fwUpdateTimepoint ) {
        m_isFirmwareUpdate = sendFirmwareUpdateRequest();
        m_fwUpdateTimepoint = ClockType::now() + FwUpdatePeriod;
    }

    if( 0 < m_checkProvisionTimepoint.time_since_epoch().count() && ClockType::now() > m_checkProvisionTimepoint ) {
        m_isCheckProvisioning = startCheckProvisionTask();
        m_checkProvisionTimepoint = ClockType::now() + ProvisionDataUpdatePeriod;
    }

    if( isWifiScanActive() && ClockType::now() - m_wifiScanTimepoint > ScanAvailableAccessPointsPeriod ) {
        tryStartWifiAccessPointsScan();
    }

#ifdef CONFIG_CLOUD_DEVICES
    if( !m_isStorageControllerTokenRequest && ( !m_storageControllerTokenRequestTimepoint ||
                                                ClockType::now() > *m_storageControllerTokenRequestTimepoint ) ) {
        requestStorageControllerToken();
    }
#endif  // CONFIG_CLOUD_DEVICES
    return ClbkRet::Continue;
}

void CloudNmaClient::onShutdownStart() {
    m_isRestartStartupService = false;
    closeConnection();
}

void CloudNmaClient::visit( const bus::BusAddr from,
                            const bus::BusAddr to,
                            bus::command::nma::ServerRequest& command ) {
    updateProvisionDataRequest();
}

void CloudNmaClient::visit( const bus::BusAddr from,
                            const bus::BusAddr to,
                            bus::command::LocalModeFeatureStatusUpdate& command ) {
    nma::BroadcastsSender::sendFeatureStatusUpdate( command.featureFlag() );
}

void CloudNmaClient::visit( const bus::BusAddr from,
                            const bus::BusAddr to,
                            bus::command::CloudDevicesFeatureStatusUpdate& command ) {
    nma::BroadcastsSender::sendFeatureStatusUpdate( command.featureFlag() );
}

void CloudNmaClient::visit( const bus::BusAddr from,
                            const bus::BusAddr to,
                            bus::command::AdvancedScenesFeatureStatusUpdate& command ) {
    nma::BroadcastsSender::sendFeatureStatusUpdate( command.featureFlag() );
}

void CloudNmaClient::visit(::bus::BusAddr,
                           ::bus::BusAddr,
                           const ::gateways::commands::ProvisioningListEntryAdded& cmd ) {
    nma::BroadcastsSender::sendProvisioningListEntryAdded( cmd.getEntry() );
}

void CloudNmaClient::visit( const bus::BusAddr from,
                            const bus::BusAddr to,
                            ::bus::command::RestoreChipBackup& command ) {
    NMA_LOGD( "RestoreChipBackup status: %d", (int)command.getStatus() );
}

void CloudNmaClient::visit( const bus::BusAddr from,
                            const bus::BusAddr to,
                            ::bus::command::CreateChipBackup& command ) {
    if( command.getStatus() == ::znet::ModeStatus::Finished ) {
        cloud_storage::BackupCloudStorage storage;
        storage.uploadFile( tokens::ChipBackupFile );
        remove( tokens::ChipBackupFile );
    }
}

void CloudNmaClient::visit(::bus::BusAddr,
                           ::bus::BusAddr,
                           const ::gateways::commands::ProvisioningListEntryUpdated& cmd ) {
    nma::BroadcastsSender::sendProvisioningListEntryUpdated( cmd.getEntryId(), cmd.getFieldDiff() );
}

void CloudNmaClient::visit(::bus::BusAddr,
                           ::bus::BusAddr,
                           const ::gateways::commands::ProvisioningListEntryRemoved& cmd ) {
    nma::BroadcastsSender::sendProvisioningListEntryRemoved( cmd.getEntryId() );
}

void CloudNmaClient::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::nma::ClientClbk& command ) {
    auto wStream = writeStream();
    command.executeClbk( *wStream );
}

void CloudNmaClient::visit( const bus::BusAddr from,
                            const bus::BusAddr to,
                            const commands::HubProvisionChanged& command ) {
    setServerInfo( command.nmaSrvName(), command.nmaSrvPort(), command.isConnectionSecured() );
    m_isNeedReconnection = true;
    m_isCheckProvisioning = false;
}

bool CloudNmaClient::tryStartWifiAccessPointsScan() {
    if( !network::wifi_srv2::startWifiScan() ) {
        return false;
    }
    m_wifiScanTimepoint = ClockType::now();
    return true;
}

bool CloudNmaClient::tryStopWifiAccessPointsScan() {
    m_wifiScanTimepoint = TimePoint{};
    return network::wifi_srv2::stopWifiScan();
}

void CloudNmaClient::visit( const bus::BusAddr from,
                            const bus::BusAddr to,
                            const network::wifi_srv2::responce::DiscoveredWifiAccessPoints& cmd ) {
    if( !isWifiScanActive() ) {
        return;
    }

    if( !cmd.isScanSuccess() ) {
        tryStopWifiAccessPointsScan();
        return;
    }

    auto ws = writeStream();
    common::JsonStreamGenerator jsGen( *ws );

    bool isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    isSuccess = isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubNetworkWifiScanProgress );
    isSuccess = isSuccess && jsGen.writePropertyKey( tokens::Result );
    isSuccess = isSuccess && jsGen.writeOpeningObject();
    {
        isSuccess = isSuccess && jsGen.writeProperty( tokens::InterfaceId, wifi::tokens::WLAN );
        isSuccess = isSuccess && jsGen.writeProperty( tokens::Status, "process" );

        isSuccess = isSuccess && jsGen.writePropertyKey( "networks" );
        isSuccess = isSuccess && jsGen.writeOpeningArray();  // [
        if( isSuccess ) {
            const auto& foundedAp = cmd.apInfo();
            for( const auto& ap : foundedAp ) {
                NMA_LOGI( "Found access point -> SSID=%s, Signal=%d, Open=%s",
                          ap.ssidToCString(),
                          ap.getRssi().rssiToPercent(),
                          ap.isOpen() ? "true" : "false" );

                isSuccess = isSuccess && jsGen.writeOpeningObject();
                {
                    isSuccess = isSuccess && jsGen.writeProperty( "ssid", ap.ssidToString() );
                    isSuccess = isSuccess && jsGen.writeProperty( "bssid", ap.bssidToHexString() );
                    isSuccess = isSuccess && jsGen.writeProperty( "signalLevel", ap.getRssi().rssiToPercent() );
                    isSuccess = isSuccess && jsGen.writeProperty( "security", ap.authModeString() );
                }
                isSuccess = isSuccess && jsGen.writeClosingSymbol();
            }
        }
        isSuccess = isSuccess && jsGen.writeClosingSymbol();  // ]
    }
    isSuccess = isSuccess && jsGen.writeClosingSymbol();
    isSuccess = isSuccess && jsGen.writeJsonEnd();

    if( !isSuccess ) {
        tryStopWifiAccessPointsScan();
    }
}

bool CloudNmaClient::sendFirmwareUpdateRequest() {
    auto ws = writeStream();
    common::JsonStreamGenerator jsGen( *ws );

    std::array<char, 32> idBuffer;

    sprintf( idBuffer.data(),
             "AT32-%" PRIX32 "-%" PRIX32,
             hard::common::generateUniqueId(),
             hard::common::generateUniqueId() );

    bool result = jsGen.writeOpeningObject();
    {
        result = result && jsGen.writeProperty( tokens::Id, idBuffer.data() );
        result = result && jsGen.writeProperty( tokens::Method, common::resources::commands::CloudFirmwareInfoGet );
        result = result && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
        result = result && jsGen.writePropertyKey( tokens::Params );
        result = result && jsGen.writeOpeningObject();
        result = result && jsGen.writeProperty( "version", FIRMWARE_VERSION );
        result = result && jsGen.writeClosingSymbol();
    }
    result = result && jsGen.writeClosingSymbol();

    result = result && jsGen.writeJsonEnd();
    if( result ) {
        NMA_LOGI( "Send firmware update request successfully" );
    }
    else {
        NMA_LOGE( "Can't send firmware update request" );
    }
    return result;
}

std::vector<char> CloudNmaClient::getUTCTimeStr() {
    auto now = std::chrono::system_clock::now();
    std::time_t tp = std::chrono::system_clock::to_time_t( now );
    constexpr auto bufSize = 32;
    std::vector<char> date( bufSize );
    std::strftime( date.data(), bufSize, "%Y-%m-%d %H:%M:%S", std::gmtime( &tp ) );
    return std::move( date );
}

}  // namespace cloud_client

#endif  // not CONFIG_UART_API
