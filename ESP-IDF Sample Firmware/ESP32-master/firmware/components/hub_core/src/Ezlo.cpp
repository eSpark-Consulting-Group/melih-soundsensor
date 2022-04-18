#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <cstdio>
#include <Utils.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/apps/sntp.h>
#include <Ezlo.h>
#include <Storage.h>
#include <StorageFile.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <ZWaveGateway.h>
#include <plugin_manager.h>
#include <FirmwareVersion.h>
#include <sdkconfig.h>
#include <PagesManager.h>
#include <sys/param.h>
#include <Mutex.h>
#include <LockKeeper.h>
#include <Volume.h>
#include <Storage.h>
#include <StoragePartitionLabel.h>
#include <BinaryFileStorageFactory.h>
#include <StorageSection.h>
#include <StorageCommonSection.h>
#include <esp_ota_ops.h>
#include <IDInfo.h>
#include <FirmwareUpdater.h>
#include "HouseModesManager.h"
#include "Cache.h"
#include "LedControl.h"

#include "BusCommandGpioButton.hpp"
#include "syscore/commands/Shutdown.h"
#include <ColudDevicesManager.h>
#include "wifi_service2/Common.h"

#include <DebugUtils.h>
#include "SettingStorage.h"

namespace core {

std::atomic_bool Ezlo::m_rebootFlag = false;

Ezlo::Ezlo( const std::chrono::seconds& shutdownDelay )
: ParentType( shutdownDelay ) {
    m_testMode = storage::Storage::get().isExist( "/vol/test.on" );
    m_button = new hard::GpioButton( static_cast<hard::GpioButton::GpioPin>( CONFIG_IO_BUTTON ) );
}

Ezlo::~Ezlo() {
    delete m_button;

    FUNC_LOGI( "Destroy Ezlo instance" );
}

void useOtherFirmwareCallback() {
    esp_err_t res = ESP_ERR_NOT_FOUND;
    const esp_partition_t* running = esp_ota_get_running_partition();
    if( running != nullptr ) {
        const esp_partition_t* next = esp_ota_get_next_update_partition( running );
        if( next != nullptr &&
            next->address != running->address ) {  //  uses 2 partitions ONLY. It triggers between each other
            res = esp_ota_set_boot_partition( next );
            if( res != ESP_OK ) {
                FUNC_LOGE( "A setting of the other partition (%s) has been FAILED", next->label );
            }
            FUNC_LOGI( "The partition (%s) has been set successfully", next->label );
        }
        else if( next != nullptr ) {
            FUNC_LOGW( "It does not have other partition." );
        }
    }
    if( res == ESP_OK ) {
        esp_restart();
    }
}

bool Ezlo::initializeImpl() {
    if( !ParentType::initializeImpl() ) {
        return false;
    }

    if( updateFailedBootCounter() > MAX_FAILED_BOOT_CNT ) {
        FUNC_LOGE( "Initiate Reset to Factory Defaults procedure" );
        startReset( ResetType::ResetToFactoryDefaults, false );
    }
    checkReset();

    checkInstallDefaultPlugins();

    core::FirmwareUpdater::checkFimwareUpdatedFlag();

    if(::core::FirmwareUpdater::wasFirmwareUpdated() ) {
        ::storage::Settings::restoreLegacyValuesFromCache();
    }

    if( !::bus::Bus::get().join(::bus::command::GPIO_BUTTON_BUS_CLIENT_NAME, *this ) ) {
        FUNC_LOGE( "Can't join to GpioButton's bus namespace." );
        return false;
    }

    auto& hmm = core::HouseModesManager::get();
    if( !hmm.initialize() ) {
        FUNC_LOGE( "Cannot initialize HouseModesManager." );
        return false;
    }

    FUNC_LOGI( "Ezlo component successfully initialized" );
    return true;
}

void Ezlo::setDescription( const char* description ) {
    const std::vector<char> buff( description, description + strlen( description ) );
    auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::COMMON );
    if( !( store && store->setBlob( storage::section::common::HUB_DESCRIPTION, buff ) ) ) {
        FUNC_LOGE( "Failed to store hub description" );
    }
}

void Ezlo::fillDescription( char* buffer, uint16_t maxSize ) {
    std::vector<char> description;
    auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::COMMON );
    if( store && store->getBlob( storage::section::common::HUB_DESCRIPTION, description ) ) {
        std::copy_n( description.begin(), std::min( description.size(), (size_t)maxSize ), buffer );
    }
    else {
        if( maxSize > 0 ) {
            buffer[0] = '\0';
        }
    }
}

const char* Ezlo::getZwaveCalibration() {
    // TODO: fill calibration info from zwave module
    return "";
}

int Ezlo::updateFailedBootCounter() {
    auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::COMMON );
    if( !store ) {
        return -1;
    }

    std::int8_t failed_boot_cnt;
    if( store->getInt8( storage::section::common::FAILED_BOOT_COUNTER, failed_boot_cnt ) ) {
        ++failed_boot_cnt;
    }
    else {
        failed_boot_cnt = 0;
    }

    FUNC_LOGD( "Failed boot counter: %d", failed_boot_cnt );
    if( !store->setInt8( storage::section::common::FAILED_BOOT_COUNTER, failed_boot_cnt ) ) {
        FUNC_LOGE( "Failed to store failed boot counter" );
    }

    return failed_boot_cnt;
}

void Ezlo::resetFailedBootCounter() {
    auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::COMMON );
    if( store ) {
        if( !store->setInt8( storage::section::common::FAILED_BOOT_COUNTER, 0 ) ) {
            FUNC_LOGE( "Failed to store failed boot counter" );
        }
    }
    else {
        FUNC_LOGE( "Failed to open storage" );
    }
}

void Ezlo::startReset( const ResetType resetType, bool restart ) {
    bool setFlag{ false };
    {
        auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::COMMON );
        if( !store ) {
            FUNC_LOGE( "Can't open storage" );
            return;
        }
        switch( resetType ) {
        case ResetType::ResetToFactoryDefaults:
            FUNC_LOGI( "Initiate reset to factory defaults" );
            hard::LedControl::onStartHardReset();
            if( !network::wifi_srv2::wipeStaConfig() ) {
                FUNC_LOGW( "Can't wipe wifi settings" );
            }
            setFlag = store->setInt8( storage::section::common::RESET_TO_FACTORY_DEFAULTS, 1 );
            break;
        case ResetType::SoftReset:
            FUNC_LOGI( "Initiate soft reset" );
            hard::LedControl::onStartSoftReset();
            setFlag = store->setInt8( storage::section::common::SOFT_RESET, 1 );
            break;
        }
    }  // close store

    if( setFlag ) {
        if( restart ) {
            reboot();
        }
        else {
            FUNC_LOGI( "Reset is scheduled for execution" );
        }
    }
    else {
        FUNC_LOGE( "Can't start reset operation" );
    }
}

void Ezlo::checkReset() {
    auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::COMMON );
    if( !store ) {
        FUNC_LOGE( "Can't open storage" );
        return;
    }
    std::int8_t resetFlag = 0;
    if( store->getInt8( storage::section::common::RESET_TO_FACTORY_DEFAULTS, resetFlag ) && resetFlag ) {
        store.reset();
        resetToFactoryDefaults();
    }
    else if( store->getInt8( storage::section::common::SOFT_RESET, resetFlag ) && resetFlag ) {
        store.reset();
        softReset();
    }
}

/**
 * @brief Reset Hub to factory default settings
 *
 * This function does not return and must be called when no other tasks are running
 *
 */
void Ezlo::resetToFactoryDefaults() {
    FUNC_LOGI( "Start reset to factory defaults" );
    hard::LedControl::onStartHardReset();

    gateways::zwave::ZNet::get().resetZwaveModule( false );
    common::waitWhileFunctionIsTrueOrTimeout( []() { return gateways::zwave::ZNet::get().isResetting(); },
                                              std::chrono::milliseconds( WaitingDelayMs ),
                                              MaxWaitingTries );
    if( !storage::Volume::FormatAllPartitions() ) {
        FUNC_LOGE( "Can't format partitions" );
    }

    // Restore system plugins
    plugin::PluginManager::get().installDefaultPlugins();
    if( !createDefaultRooms() ) {
        FUNC_LOGI( "Failed to create default rooms" );
    }
    hard::LedControl::onResetToFactoryDefaults();
}

/**
 * @brief Clean hub but left it connected to network
 *
 * This function does not return and must be called when no other tasks are running
 *
 */
void Ezlo::softReset() {
    FUNC_LOGI( "Start soft reset" );
    hard::LedControl::onStartSoftReset();

    gateways::zwave::ZNet::get().resetZwaveModule( false );
    common::waitWhileFunctionIsTrueOrTimeout( []() { return gateways::zwave::ZNet::get().isResetting(); },
                                              std::chrono::milliseconds( WaitingDelayMs ),
                                              MaxWaitingTries );
    // Remove user accounts
    // Clear security settings

    bool result = true;
    if( !storage::Volume::FormatAllPartitions() ) {
        FUNC_LOGE( "Can't format partition" );
        result = false;
    }

    // Restore system plugins
    plugin::PluginManager::get().installDefaultPlugins();

    if( !createDefaultRooms() ) {
        FUNC_LOGI( "Failed to create default rooms" );
        result = false;
    }

    if( !result ) {
        FUNC_LOGE( "Failed to complete soft reset" );
        resetToFactoryDefaults();
    };
    hard::LedControl::onResetToFactoryDefaults();
}

void Ezlo::checkInstallDefaultPlugins() {
    auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::COMMON );
    if( !store ) {
        FUNC_LOGE( "Can't open storage" );
        return;
    }
    std::int8_t value;
    if( store->getInt8( storage::section::common::REINSTALL_DEFAULT_PLUGINS, value ) ) {
        installDefaultPlugins();
        if( !store->remove( storage::section::common::REINSTALL_DEFAULT_PLUGINS ) ) {
            FUNC_LOGE( "Failed to clear flag to reinstall default plugins" );
        }
    }
    else {
        installDefaultPlugins( false );
    }
}

void Ezlo::forceReinstallDefaultPlugins() {
    auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::COMMON );
    if( !store ) {
        FUNC_LOGE( "Can't open storage" );
        return;
    }
    FUNC_LOGI( "Reinstall default plugins on next reboot" );
    if( !store->setInt8( storage::section::common::REINSTALL_DEFAULT_PLUGINS, 1 ) ) {
        FUNC_LOGE( "Failed to set flag to reinstall default plugins" );
    };
}

void Ezlo::installDefaultPlugins( bool forceReinstall ) {
    if( !plugin::PluginManager::get().installDefaultPlugins( forceReinstall ) ) {
        FUNC_LOGE( "Failed to reinstall default plugins" );
    }
}

const char* Ezlo::uuid() {
    static char uuidBuffer[UuidWidth + 1] = {};
    if( uuidBuffer[0] == '\0' ) {
        fillUuid( uuidBuffer, sizeof( uuidBuffer ) );
    }
    return uuidBuffer;
}

int Ezlo::fillUuid( char* const buffer, uint16_t bufferSize ) {
    return common::uuidBinToStr( core::IDInfo::get().uuid().data(), buffer, bufferSize );
}

const char* Ezlo::hubType() {
    return "32.1";
}

int Ezlo::getTimeOffset() {
    time_t now;

    time( &now );
    struct tm* timeinfo = localtime( &now );

#ifdef HAVE_STRUCT_TM_TM_GMTOFF
    auto gmtoff = timeinfo->tm_gmtoff;
#else  // !HAVE_STRUCT_TM_TM_GMTOFF
    auto gmtoff = common::nghttp2_timegm( timeinfo ) - now;
#endif

    return (int)( gmtoff );
}

void Ezlo::reboot() {
    send<core::commands::Shutdown>( core::commands::BUS_NAME );
}

int64_t Ezlo::getUptime() {
    return esp_timer_get_time() / 1'000'000;
}

void Ezlo::setLatestVersion( const char* version ) {
    if( version == nullptr || version[0] == '\0' ) {
        return;
    }

    strncpy( m_latestFirmwareVersion, version, MaxVersionLength );
}

const char* Ezlo::getLatestVersion() {
    return ( m_latestFirmwareVersion[0] == '\0' ) ? FIRMWARE_VERSION : m_latestFirmwareVersion;
}

bool Ezlo::createDefaultRooms() {
    core::PagesManager::PageId pageId = 0;
    int errorCode;
    auto pagesManager = core::PagesManager::get();
    if( !pagesManager ) {
        FUNC_LOGI( "Can't get PagesManager instance" );
        return false;
    }
    std::tie( errorCode, std::ignore ) = pagesManager->createPage( core::PagesManager::BedroomName, pageId );
    if( common::resources::errors::NotError != errorCode ) {
        return false;
    }
    std::tie( errorCode, std::ignore ) = pagesManager->createPage( core::PagesManager::LivingroomName, pageId );
    return common::resources::errors::NotError == errorCode;
}

#if not CONFIG_BOARD_TYPE_PLUG_HUB
void Ezlo::visit(::bus::BusAddr from, ::bus::BusAddr to, bus::command::GpioButtonClicked& command ) {
    FUNC_LOGI( "Send NIF" );
    hard::LedControl::onSendNif();
}

void Ezlo::visit(::bus::BusAddr from, ::bus::BusAddr to, bus::command::GpioButtonLongClicked& command ) {
    FUNC_LOGI( "Soft Reset" );
    startReset( core::Ezlo::ResetType::SoftReset );
}
#endif  // CONFIG_BOARD_TYPE_PLUG_HUB

void Ezlo::visit(::bus::BusAddr from, ::bus::BusAddr to, bus::command::GpioButtonVeryVeryLongClicked& command ) {
    FUNC_LOGI( "Reset to factory defaults" );
    startReset( core::Ezlo::ResetType::ResetToFactoryDefaults );
}

void Ezlo::visit(::bus::BusAddr from, ::bus::BusAddr to, bus::command::GpioButtonVeryLongClicked& command ) {
#if CONFIG_BOARD_TYPE_PLUG_HUB
    FUNC_LOGI( "Soft Reset" );
    startReset( core::Ezlo::ResetType::SoftReset );
#else
    FUNC_LOGI( "Reset to factory defaults" );
    startReset( core::Ezlo::ResetType::ResetToFactoryDefaults );
#endif
}

void Ezlo::onShutdownStart() {
    ParentType::onShutdownStart();
    m_rebootFlag = true;
}

Ezlo::ClbkRet Ezlo::updateComponent( UpdContext& ctx ) {
    m_button->update();

    core::HouseModesManager::get().update();

    ctx.tryYield();

    if( !m_isFailedBootCtrReseted ) {
        if( xTaskGetTickCount() > MIN_NORMAL_REBOOT_TIMEOUT_MS / portTICK_PERIOD_MS ) {
            FUNC_LOGD( "Reset failed boot counter" );
            resetFailedBootCounter();
            m_isFailedBootCtrReseted = true;
        }
    }

    return ClbkRet::Continue;
}

}  // namespace core
