#pragma once

#include <stdint.h>
#include <string>
#include <memory>
#include <ctime>
#include <cstdarg>
#include <vector>
#include <GpioButton.h>
#include "Shutdownable.h"
#include "lua/responce/HubSendBroadcast.h"
#include "hub_core/Common.h"

namespace storage {
class File;
}  // namespace storage

namespace core {

class Ezlo : public SingletonBaseImpl<Ezlo> {
    friend class SingletonBaseImpl<Ezlo>;

public:
    using ParentType = SingletonBaseImpl<Ezlo>;
    using ClbkRet = ParentType::ClbkRet;
    using UpdContext = ParentType::UpdContext;

    static constexpr size_t MaxDescriptonSize = 64;
    static constexpr size_t UuidWidth = 36;
    static constexpr size_t MaxVersionLength = 32;
    static constexpr size_t MaxCoordinateLength = 24;
    static constexpr auto MaxWaitingTries = 50;
    static constexpr auto WaitingDelayMs = 100;

    static constexpr auto MAX_FAILED_BOOT_CNT = 10;
    static constexpr auto MIN_NORMAL_REBOOT_TIMEOUT_MS = 60000;

    enum class ResetType { ResetToFactoryDefaults, SoftReset };

public:
    ~Ezlo();

    void setDescription( const char* description );
    void fillDescription( char* buffer, uint16_t maxSize );

    const char* getZwaveCalibration();

    void setLatestVersion( const char* version );
    const char* getLatestVersion();

    void startReset( const ResetType resetType, bool restart = true );
    void checkReset();
    void resetToFactoryDefaults();
    void softReset();
    void reboot();

    void forceReinstallDefaultPlugins();
    void checkInstallDefaultPlugins();
    void installDefaultPlugins( bool forceReinstall = true );

    static const char* uuid();
    static int fillUuid( char* const buffer, uint16_t bufferSize );

    int getTimeOffset();
    const char* hubType();

    static inline bool isShuttingDown() {
        return m_rebootFlag;
    }

    int64_t getUptime();

    static inline auto getOfflineServerPort() {
        return m_offlineServerPort;
    }

    // bus::IBusCommandVisitor
#if not CONFIG_BOARD_TYPE_PLUG_HUB
    void visit( const ::bus::BusAddr from,
                const ::bus::BusAddr to,
                ::bus::command::GpioButtonClicked& command ) override;

    void visit( const ::bus::BusAddr from,
                const ::bus::BusAddr to,
                ::bus::command::GpioButtonLongClicked& command ) override;
#endif  // CONFIG_BOARD_TYPE_PLUG_HUB

    void visit( const ::bus::BusAddr from,
                const ::bus::BusAddr to,
                ::bus::command::GpioButtonVeryVeryLongClicked& command ) override;

    void visit( const ::bus::BusAddr from,
                const ::bus::BusAddr to,
                ::bus::command::GpioButtonVeryLongClicked& command ) override;

    void onShutdownStart() override;

    ClbkRet updateComponent( UpdContext& ctx ) override;

#ifdef CONFIG_UART_API
    bool m_isFailedBootCtrReseted = false;
#endif

private:
    Ezlo( const std::chrono::seconds& shutdownDelay );

    bool initializeImpl() override;

    bool getNmaServerSecurity();

    bool createDefaultRooms();

    static int updateFailedBootCounter();

    void resetFailedBootCounter();

private:
    static std::atomic_bool m_rebootFlag;
    constexpr static uint16_t m_offlineServerPort = 17000;

    char m_latestFirmwareVersion[MaxVersionLength] = "";

    uint16_t m_nmaPort = 0;

    bool m_testMode = false;

    hard::GpioButton* m_button = nullptr;

#ifndef CONFIG_UART_API
    bool m_isFailedBootCtrReseted = false;
#endif
};

}  // namespace core
