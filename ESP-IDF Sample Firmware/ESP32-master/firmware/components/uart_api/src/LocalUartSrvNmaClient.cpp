#include "LocalUartSrv.h"

#include <NmaCommand.h>
#include "UNmaJsonHandler.h"

#include <Storage.h>
#include <BinaryFileStorageFactory.h>
#include <StorageSection.h>
#include <ObjectSerializer.h>
#include "INmaJsonHandler.h"

#include "UJsonSocket.h"

#include <CommonResources.h>

#include <JsonStreamGenerator.h>
#include <CommonResources.h>
#include <GatewayUtils.h>

#include <FirmwareVersion.h>
#include <Utils.h>
#include <HardUtils.h>
#include <Ezlo.h>
#include <IDInfo.h>

#include "BusCommandNmaRequestTimeZones.h"

#include "lua/responce/AddDevice.h"
#include "lua/responce/UpdateDevice.h"
#include "lua/responce/RemoveDevice.h"
#include "lua/responce/AddItem.h"
#include "lua/responce/UpdateItemDictionary.h"
#include <BusCommandRemoveItem.h>
#include <BusCommandUpdateGateway.h>
#include "lua/responce/HubSendBroadcast.h"
#include <BusCommandModesAlarmsOffDefault.h>
#include <BusCommandModesAlarmsOffAdded.h>
#include <BusCommandModesAlarmsOffRemoved.h>
#include <BusCommandModesChanged.h>
#include <BusCommandModesDisarmedDevicesDefault.h>
#include <BusCommandModesDisarmedDevicesAdded.h>
#include <BusCommandModesDisarmedDevicesRemoved.h>
#include <BusCommandModesNotificationsNotifyAll.h>
#include <BusCommandModesNotificationsAdded.h>
#include <BusCommandModesNotificationsRemoved.h>
#include <BusCommandModesSwitched.h>
#include <BusCommandRoomsCreated.h>
#include <BusCommandRoomsDeleted.h>
#include <BusCommandRoomsReordered.h>
#include "scenes/responces/SceneDeleted.h"
#include "scenes/responces/SceneRunProgress.h"

#if CONFIG_REMOTE_LOGGING
#ifdef IS_SAVE_CRASH_DUMP
#include <BusCommandSendCrashLog.h>
#endif  // IS_SAVE_CRASH_DUMP
#endif  // CONFIG_REMOTE_LOGGING

#include <BusCommandRemoteLog.h>
#include "BusCommandFavoriteUpdated.h"
#include "BusCommandNmaClientClbk.h"
#include <BusCommandSetHandler.h>
#include <BusCommandHandleEventUpdateProductName.hpp>
#include <BusCommandFeatureFlag.hpp>

#include <values/BoolValue.h>
#include <IValueCast.h>

#include "USocket.h"

#include <memory>
#include <chrono>
#include <thread>

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

#define ENABLE_NMA_LOG
#ifdef ENABLE_NMA_LOG

#include "DebugUtils.h"

#define NMA_CLIENT_TAG "[UART-CloudNmaClient"
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

namespace ucloud_client {
namespace {

using namespace common::resources;

constexpr uint16_t MAX_NMA_CONNECTION_ATTEMPTS = 2;
constexpr uint16_t MAX_NMA_CONNECTION_ATTEMPTS_TOTAL = 2;

inline bool writeServiceNotification( const std::string& deviceId, common::JsonStreamGenerator& gen ) {
    bool isServiceNotification = common::isServiceNotification( deviceId );
    return gen.writeProperty( common::resources::tokens::ServiceNotification, isServiceNotification );
}

}  // namespace

UCloudNmaClient::UCloudNmaClient( std::shared_ptr<::core::EspLogger> espLogger,
                                  ::gateways::IGatewayUserDataStorage& gatewayUserDataStorage )
: nma::NmaBase{ createSocket(), espLogger, gatewayUserDataStorage }
, m_eventsHandler{ static_cast<bus::BusAddr>( 0 ), std::string( "" ) } {
    NMA_LOGI( "Constructor(...)" );
}

network::ISocket::UPtr UCloudNmaClient::createSocket() {
    auto tcpSock = std::make_unique<network::USocket>( 1 );
    return std::make_unique<network::UJsonSocket>( std::move( tcpSock ) );
}

UCloudNmaClient::~UCloudNmaClient() {
    NMA_LOGI( "Destructor()" );
}

void UCloudNmaClient::onConnected() {
    m_isConnected = true;
    joinToBus();
    getAbstractList();
}

void UCloudNmaClient::onConnLost() {}

bool UCloudNmaClient::initializeImpl() {
    if( !m_espLogger || !NmaBase::initializeImpl() ) {
        return false;
    }

    setJsonHandler( std::make_unique<UNmaJsonHandler>( *this, m_espLogger, m_gatewayUserDataStorage ) );

    return true;
}

bool UCloudNmaClient::joinToBus() {
    return NmaBase::joinToBus() && bus::Bus::get().join(::bus::command::FEATURE_FLAG_BUS_CLIENT_NAME, *this );
}

void UCloudNmaClient::connectToNma( const std::string& srvName, const unsigned int srvPort ) {
    if( !m_socket->connectToHost( srvName, srvPort ) ) {
        vTaskDelay( 300 / portTICK_PERIOD_MS );
        return;
    }

    m_isConnected = true;
    if( sendRegisterHubRequest() ) {
        connectToNmaFlow();
        m_isConnectionSecured = true;
    }
}

bool UCloudNmaClient::sendRegisterHubRequest() {
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
        result = result && jsGen.writeProperty( tokens::MacAddress, "00:00:00:00:00:00" );
    }
    result = result && jsGen.writeClosingSymbol();

    return result && jsGen.writeJsonEnd();
}

void UCloudNmaClient::onRegistered() {
    sendFirmwareUpdateRequest();
    m_fwUpdateTimepoint = std::chrono::steady_clock::now() + FwUpdatePeriod;
}

bool UCloudNmaClient::tryConnectToNma() {
    onConnected();
    return true;
}

void UCloudNmaClient::getNewNmaAddress( bool force ) {
    m_isConnectionSecured = true;
    return;
}

void UCloudNmaClient::connectToNmaFlow() {
    tryConnectToNma();
}

void UCloudNmaClient::updateProvisionDataRequest() {
    m_nmaServer.clear();
    m_nmaPort = 0;

    m_provisionUpdated = false;
}

bool UCloudNmaClient::updateProvisionDataIfNeeded() {
    core::IDInfo::get().init();

    return true;
}

void UCloudNmaClient::getAbstractList() {
    auto ws = writeStream();
    if( !ws ) {
        return;
    }

    common::JsonStreamGenerator gen( *ws );
    gen.writeProperty( tokens::Id, core::Ezlo::uuid() ) &&
        gen.writeProperty( tokens::Method, common::resources::commands::CloudControllerAbstractsList ) &&
        gen.writeProperty( tokens::Api, "1" ) && gen.writePropertyKey( tokens::Params ) && gen.writeOpeningObject() &&
        gen.writeProperty( tokens::ExtensionVersion, -1 ) && gen.writeClosingSymbol() && gen.writeJsonEnd();
}

UCloudNmaClient::ClbkRet UCloudNmaClient::update( UpdContext& ctx ) {
    static uint start = 0;
    m_connectionAttempts = 0;
    if( !start ) {
        core::IDInfo::get().init();
        connectToNma( "", 0 );
        start = 1;
    }

    {
        auto res = nma::NmaBase::update( ctx );
        if( ClbkRet::Completed == res ) {
            onConnLost();
        }
    }
    return jsonHandler().parse( *m_readStream, writeStream() ) ? ClbkRet::Continue : ClbkRet::Completed;
}

void UCloudNmaClient::shutdown() {
    m_socket->close();
}

void UCloudNmaClient::visit( const bus::BusAddr from,
                             const bus::BusAddr to,
                             bus::command::nma::ServerRequest& command ) {
    getNewNmaAddress( true );
}

void UCloudNmaClient::visit( const bus::BusAddr from,
                             const bus::BusAddr to,
                             bus::command::HandleEventUpdateProductName& command ) {
    send<bus::command::HandleEventUpdateProductName>(
        m_eventsHandler.m_addr, m_eventsHandler.m_handlerName, "product_name_updated", 0 );
}

void UCloudNmaClient::visit( const bus::BusAddr from,
                             const bus::BusAddr to,
                             bus::command::LocalModeFeatureStatusUpdate& command ) {
    nma::BroadcastsSender::sendFeatureStatusUpdate( command.featureFlag() );
}

void UCloudNmaClient::visit( const bus::BusAddr from,
                             const bus::BusAddr to,
                             bus::command::CloudDevicesFeatureStatusUpdate& command ) {
    nma::BroadcastsSender::sendFeatureStatusUpdate( command.featureFlag() );
}

void UCloudNmaClient::visit( const bus::BusAddr from,
                             const bus::BusAddr to,
                             bus::command::AdvancedScenesFeatureStatusUpdate& command ) {
    nma::BroadcastsSender::sendFeatureStatusUpdate( command.featureFlag() );
}

void UCloudNmaClient::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::nma::ClientClbk& command ) {
    auto wStream = writeStream();
    command.executeClbk( *wStream );
}

void UCloudNmaClient::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SetHandler& command ) {
    if( command.isSubscribe() ) {
        auto addr = bus::BusAddrInvalid == command.addr() ? from : command.addr();
        m_eventsHandler = { addr, command.handlerName() };

        if( m_deferredSend.size() > 0 && !m_eventsHandler.m_handlerName.empty() ) {
            while( !m_deferredSend.empty() ) {
                auto bus_command = std::move( m_deferredSend.front() );
                if( bus_command ) {
                    m_deferredSend.pop();
                    bus_command->handlerName( m_eventsHandler.m_handlerName );
                    send( m_eventsHandler.m_addr, std::move( bus_command ) );
                }
            }
        }
    }
    else {
        m_eventsHandler = { bus::BusAddrInvalid, "" };
    }
}

bool UCloudNmaClient::sendFirmwareUpdateRequest() {
    auto ws = writeStream();
    common::JsonStreamGenerator jsonGenerator( *ws );

    std::array<char, 32> idBuffer;

    sprintf( idBuffer.data(),
             "AT32-%" PRIX32 "-%" PRIX32,
             hard::common::generateUniqueId(),
             hard::common::generateUniqueId() );

    bool result = jsonGenerator.writeOpeningObject();
    {
        result = result && jsonGenerator.writeProperty( tokens::Id, idBuffer.data() );
        result =
            result && jsonGenerator.writeProperty( tokens::Method, common::resources::commands::CloudFirmwareInfoGet );

        result = result && jsonGenerator.writePropertyKey( tokens::Params );
        result = result && jsonGenerator.writeOpeningObject();
        { result = result && jsonGenerator.writeProperty( "version", FIRMWARE_VERSION ); }
        result = result && jsonGenerator.writeClosingSymbol();
    }
    result = result && jsonGenerator.writeClosingSymbol();

    return result && jsonGenerator.writeJsonEnd();
}

}  // namespace ucloud_client
