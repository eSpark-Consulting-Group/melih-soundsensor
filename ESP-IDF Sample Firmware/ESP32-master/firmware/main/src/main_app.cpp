#include "sdkconfig.h"
#ifndef IS_FACTORY_TEST
#include <Defs.h>

#include <SystemStorage.h>
#include <DebugUtils.h>

#include "hub_core/InitializeComponent.h"
#include "LuaEventManager.h"
#include <RemoteLogger.h>
#include <Utils.h>
#include <Storage.h>
#include <Volume.h>
#include <Cache.h>
#include <GatewayTask.h>
#include <IGateway.h>
#include "GatewayUserDataInMemoryStorage.hpp"
#include <Lua.h>
#include <GatewayDeviceRestore.h>
#include <chrono>
#include <ctime>
#include <FeatureFlagController.h>
#include <EspLogger.h>
#include <StorageSection.h>

#include <CommonResources.h>
#include <GatewayUtils.h>
#ifdef CONFIG_BOARD_TYPE_WATER_VALVE
#include <WaterValve.h>
#endif
#include "DeviceInfo.h"
#include "DeviceSettingsInfo.hpp"
#include "DeviceSettingsManager.h"
#include "TCPServer2.h"
#include "IDInfo.h"
#include "FirmwareUpdater.h"
#include "UpdatableService.h"
#include "Thread.h"
#include "SettingStorage.h"

#ifdef CONFIG_UART_API
#include "LocalUartSrvTask.h"
#else  // not CONFIG_UART_API
#include "scenes/InitializeComponent.h"
#include "scenes/Utils.h"
#include "wifi_service2/Common.h"
#include "services/WifiBootstrapTask.h"
#include "time_service/InitializeComponent.h"
#endif  // CONFIG_UART_API

#include "services/MemInfoLogService.h"
#include "LedControl.h"

#ifdef CONFIG_LOCAL_MODE
#include "local_nma_srv/InitializeComponent.h"
#endif  // CONFIG_LOCAL_MODE

#ifdef HIMEM
#include <esp_himem.h>
#endif

#ifdef CONFIG_BOARD_TYPE_PLUG_HUB
#include "Meter_cc.h"
#endif

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
#include "dev_dis/InitializeComponent.h"
#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

using namespace std::chrono_literals;

std::list<core::Thread> g_threads;

extern void logInfo();

void onInitializationFailed( const char* msg ) {
    FUNC_LOGE( "Fail message: %s", msg );
    constexpr auto wait_period = std::chrono::seconds( 5 );
    auto timePoint = std::chrono::steady_clock::now() + wait_period;
    while( timePoint > std::chrono::steady_clock::now() ) {
        // Do nothing
    }
    esp_restart();
}

#define CHECK_INIT( _RESULT_, _MSG_ )        \
    do {                                     \
        if( !( _RESULT_ ) ) {                \
            onInitializationFailed( _MSG_ ); \
        }                                    \
    } while( false );

extern "C" void app_main() {
    {
        std::chrono::seconds ledControlShutdownDelay( 0 );
        auto ledControl = hard::LedControl::createSingleton( std::move( ledControlShutdownDelay ) );
        CHECK_INIT( bool( ledControl ), "Can't create LedControl singleton!" );

        CHECK_INIT( storage::Volume::Init(), "storage::Volume::Init() failed!" );

        cache::Cache::init();

        {
            auto& idInfo = core::IDInfo::get();
            CHECK_INIT( idInfo.init(), "Can't init IDInfo!" );
        }

#if CONFIG_REMOTE_LOGGING
        {
            std::chrono::seconds loggerShutdownDelay( 1 );
            auto logger = core::Logger::createSingleton( std::move( loggerShutdownDelay ) );
            CHECK_INIT( bool( logger ), "Can't init remote logger!" );
            core::upd_srv::add( std::move( logger ), "Logger" );
        }
#endif  // CONFIG_REMOTE_LOGGING

        logInfo();

        std::atomic<bool> isRunMainCycle{ false };

#ifdef CONFIG_UART_API
        ledControl->setIsWifiConfigExist( true );
#else   // not CONFIG_UART_API
        ledControl->setIsWifiConfigExist( network::wifi_srv2::hasValidStaConfig() );
#endif  // CONFIG_UART_API
        core::upd_srv::add( std::move( ledControl ), "LedControl" );

#ifdef CONFIG_BOARD_TYPE_PLUG_HUB
        Meter_cc::get().init_meter();
#endif  // CONFIG_BOARD_TYPE_PLUG_HUB

#ifdef CONFIG_BOARD_TYPE_WATER_VALVE
        gateways::water_valve::WaterValve::get().initialize_hw();
#endif

        ::gateways::GatewayUserDataInMemoryStorage gatewayUserDataInMemoryStorage;

        CHECK_INIT( core::initializeComponent(), "Can't initialize \"core\" component" );
#ifndef CONFIG_UART_API
        CHECK_INIT( time_service::initializeComponent(), "Can't initialize \"time_service\" component" );
#endif  // CONFIG_UART_API

        auto espLogger = std::make_shared<::core::EspLogger>(
            ::storage::BinaryFileStorageFactory::get().openNamespace(::storage::section::ESP_LOGGER ) );

#ifndef CONFIG_UART_API
        {
            core::Thread wifiBootstrapThread( "WifiBootstrap",
                                              4 * 1024,
                                              sys_serv::wifiBootstrapTask,
                                              std::ref( g_threads ),
                                              espLogger,
                                              std::ref( gatewayUserDataInMemoryStorage ),
                                              std::ref( isRunMainCycle ) );
            CHECK_INIT( wifiBootstrapThread, "Can't run WifiBootstrap thread task!" );
            wifiBootstrapThread.detach();
        }
#endif  // not CONFIG_UART_API

#ifndef NO_GW
        gateways::restoreDevicesInfo();
#endif  // NO_GW
        {
            std::chrono::seconds featureFlagControllerShutdownDelay( 1 );
            auto featureFlagController = ::core::features::FeatureFlagController::createSingleton(
                ::storage::BinaryFileStorageFactory::get().openNamespace(::storage::section::COMMON ),
                featureFlagControllerShutdownDelay );

            CHECK_INIT( bool( featureFlagController ), "Cannot initialize ::core::features::FeatureFlagController" );
            core::upd_srv::add( std::move( featureFlagController ), "FeaturesController" );
        }

#ifndef NO_GW
#ifdef RUN_GATEWAYS
        {
            bool isInitSuccess = true;
            gateways::enumerateGateways( [&isInitSuccess, &gatewayUserDataInMemoryStorage](
                                             gateways::GatewayID gatewayId, gateways::IGateway* gateway ) {
                FUNC_LOGD( "Try to init gateway: %s(Id: %d, Ptr: %p)",
                           gateways::toString( gatewayId ),
                           (int)gatewayId,
                           gateway );
                isInitSuccess = gateway && gateway->initialize();
                FUNC_LOGE( "return isInitSuccess: %d", isInitSuccess );
                if( auto gatewayUserData = ::gateways::utils::loadGatewayUserDataFromConfigJson( gatewayId ) ) {
                    gatewayUserDataInMemoryStorage.setUserData( gatewayId, std::move( gatewayUserData ) );
                }
                return isInitSuccess;
            } );
            CHECK_INIT( isInitSuccess, "Can't init gateways!" );
        }
#endif  // RUN_GATEWAYS
#endif  // NO_GW

        {
            std::chrono::seconds luaShutdownDelay( 3 );
            auto luaEventManager = std::make_unique<lua::LuaEventManager>( std::move( luaShutdownDelay ) );
            CHECK_INIT( luaEventManager && luaEventManager->initialize(),
                        "Can't initialize LuaEventManager instance!" );
            core::upd_srv::add( std::move( luaEventManager ), "LuaEventManager" );
        }

        std::chrono::seconds luaExecutorShutdownDelay( 2 );
#ifdef CONFIG_SPIRAM
        constexpr auto luaExecutorMsgQueueSize = 512;
#else   // not CONFIG_SPIRAM
        constexpr auto luaExecutorMsgQueueSize = 256;
#endif  // CONFIG_SPIRAM
        auto luaExecutor = lua::Executor::createSingleton(
            gatewayUserDataInMemoryStorage, luaExecutorShutdownDelay, luaExecutorMsgQueueSize );
        CHECK_INIT( bool( luaExecutor ), "Can't init lua::Executor!" );

#ifndef NO_GW
#ifdef RUN_GATEWAYS
        auto scriptStartup =
            [&luaExecutor]( const std::string& scriptName ) -> gateways::IGateway::ExecuteScriptResult {
            return luaExecutor->execScript( scriptName, []( lua_State* ) -> int { return 0; } );
        };
        gateways::enumerateGateways( [&scriptStartup]( gateways::GatewayID getawayId, gateways::IGateway* gateway ) {
            FUNC_LOGD( "Try to startup gateway: %s(Id: %d, Ptr: %p)",
                       gateways::toString( getawayId ),
                       (int)getawayId,
                       gateway );
            gateway->startup( scriptStartup );
            return true;
        } );
#endif  // RUN_GATEWAYS
#endif  // NO_GW

        if( luaExecutor->initializeMods() ) {
            FUNC_LOGI( "Lua modules was successfully initialized" );
        }
        else {
            FUNC_LOGE( "Can't initialize Lua modules!" );
        }

#ifndef NO_GW
#ifdef RUN_GATEWAYS
        gateways::enumerateGateways( [&g_threads]( gateways::GatewayID getawayId, gateways::IGateway* gateway ) {
            FUNC_LOGD(
                "Try to run gateway: %s(Id: %d, Ptr: %p)", gateways::toString( getawayId ), (int)getawayId, gateway );
            core::Thread gatewayThread( gateways::toString( getawayId ),
                                        gateway->getRecommendedThreadStackSize(),
                                        gateways::threadTask,
                                        gateway );
            if( gatewayThread ) {
                g_threads.push_back( std::move( gatewayThread ) );
            }
            else {
                FUNC_LOGI( "Can't run gateways::threadTask for %s", gateways::toString( getawayId ) );
            }
            return true;
        } );
#endif  // RUN_GATEWAYS
#endif  // NO_GW

#ifndef CONFIG_UART_API
        {
            bool shouldRebuildScenesBlocks = core::FirmwareUpdater::wasFirmwareUpdated();
            if( !scenes::updateHubScenesTriggerDevicesList( shouldRebuildScenesBlocks ) ) {
                FUNC_LOGW( "Can't update scenes templates!" );
            }
        }
#endif  // not CONFIG_UART_API

        {
            core::Thread luaThread( "LUA_TASK", 8 * 1024, lua::Executor::threadTask, std::move( luaExecutor ) );
            CHECK_INIT( luaThread, "Can't run lua::Executor::threadTask!" );
            g_threads.push_back( std::move( luaThread ) );
        }

#ifndef CONFIG_UART_API
        CHECK_INIT( scenes::initializeComponent( g_threads ), "Can't initialize scenes" );
#endif  // not CONFIG_UART_API

#ifdef CONFIG_LOCAL_MODE
        CHECK_INIT( local_nma_srv::initializeComponent( espLogger, gatewayUserDataInMemoryStorage, g_threads ),
                    "Can't initialize local nma server" );
#endif  // CONFIG_LOCAL_MODE

#ifdef CONFIG_UART_API
        {
            core::Thread ucloudClientThread( "CLOUD_CLIENT",
                                             4 * 1024,
                                             ucloud_client::threadTask,
                                             espLogger,
                                             std::ref( gatewayUserDataInMemoryStorage ) );
            CHECK_INIT( ucloudClientThread, "Can't run cloud_client::threadTask!" );
            g_threads.push_back( std::move( ucloudClientThread ) );
        }
#endif  // CONFIG_UART_API

#ifdef SHOW_AVERAGE
        {
            std::chrono::seconds memInfoLogServiceShutdownDelay( 0 );
            auto memInfoService = std::make_unique<sys_serv::MemInfoLogService>( memInfoLogServiceShutdownDelay );
            CHECK_INIT( bool( memInfoService ) && memInfoService->initialize(), "Can't MemInfoLogService!" );
            core::upd_srv::add( std::move( memInfoService ), "MemInfoLogService" );
        }
#endif  // SHOW_AVERAGE

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
        CHECK_INIT( dev_dis::initializeComponent(), "Can't initialize \"Devices discovery\" component" );
#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

        isRunMainCycle = true;
        core::upd_srv::loop();

        FUNC_LOGI( "Join all threads" );
        for( auto& th : g_threads ) {
            FUNC_LOGI( "Try join to thread %s", th.name().c_str() );
            th.join();
        }
        g_threads.clear();

        storage::Volume::Deinit();
    }

    FUNC_LOGI( "Delay 500 msec before reboot" );
    vTaskDelay( 500 / portTICK_PERIOD_MS );
    esp_restart();
}

#endif
