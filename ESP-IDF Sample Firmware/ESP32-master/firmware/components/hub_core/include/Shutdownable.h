#pragma once

#include "Bus.h"
#include "UpdatableService.h"

#include <mutex>
#include <memory>

namespace core {

class Shutdownable : public bus::IBusClient, public upd_srv::IUpdatable, public common::IInitialize {
public:
    using ParentType = upd_srv::IUpdatable;
    using ClbkRet = upd_srv::ClbkRet;

    using ClockType = ParentType::ClockType;
    using TimePoint = ParentType::TimePoint;
    using UpdContext = upd_srv::UpdContext;

    enum class ShutdownState : int { Active, ShutdownStart, Shutdowned };

    static constexpr auto DEFAULT_SHUTDOWN_DELAY = std::chrono::seconds( 2 );

    Shutdownable( const unsigned int msgLen = bus::IBusClient::DEFAULT_MSG_QUEUE_SIZE )
    : IBusClient( msgLen )
    , m_shutdownDelay( DEFAULT_SHUTDOWN_DELAY ){};

    template <typename Rep, typename Period>
    Shutdownable( const std::chrono::duration<Rep, Period>& delay,
                  const unsigned int msgLen = bus::IBusClient::DEFAULT_MSG_QUEUE_SIZE )
    : IBusClient( msgLen )
    , m_shutdownDelay( delay ) {}

    auto isShutdowned() const {
        return ShutdownState::Shutdowned == m_shutdownState;
    }

    auto isShutdownStart() const {
        return ShutdownState::ShutdownStart == m_shutdownState;
    }

    void startShutdown();

public:  // upd_srv::IUpdatable
    ClbkRet update( UpdContext& ctx ) override;

protected:
    void visit( const bus::BusAddr from, const bus::BusAddr to, const commands::Shutdown& command ) override;

    bool initializeImpl() override;

    ClbkRet updateShutdownState();

    virtual void onShutdownStart(){};

    virtual void onShutdownComplete(){};

private:
    const std::chrono::milliseconds m_shutdownDelay;
    ShutdownState m_shutdownState = ShutdownState::Active;
};  // class Shutdownable

template <typename ClassType>
class SingletonBaseImpl : public Shutdownable {
public:
    using ParentType = Shutdownable;
    using ClbkRet = ParentType::ClbkRet;

    static inline std::shared_ptr<ClassType> get() {
        const std::lock_guard<std::mutex> guard( singletonMutex() );
        return singletonInstanceWPtr().lock();
    }

    template <typename... Args>
    static std::shared_ptr<ClassType> createSingleton( Args&&... args ) {
        const std::lock_guard<std::mutex> guard( singletonMutex() );

        if( singletonInstanceWPtr().lock() ) {
            return nullptr;
        }

        auto ptr = new ClassType( std::forward<Args>( args )... );
        if( !ptr ) {
            return nullptr;
        }
        std::shared_ptr<ClassType> instance( ptr );

        if( !instance->initialize() ) {
            return nullptr;
        }

        singletonInstanceWPtr() = instance;

        return std::move( instance );
    }

protected:  // Shutdownable
    void onShutdownComplete() override {
        const std::lock_guard<std::mutex> guard( singletonMutex() );

        if( isShutdowned() ) {
            return;
        }

        singletonInstanceWPtr().reset();
    }

private:
    using Shutdownable::Shutdownable;

    static inline auto& singletonMutex() {
        static std::mutex g_singletonInstanceMutex;
        return g_singletonInstanceMutex;
    }

    static inline auto& singletonInstanceWPtr() {
        static std::weak_ptr<ClassType> g_singletonInstance;
        return g_singletonInstance;
    }
};

}  // namespace core
