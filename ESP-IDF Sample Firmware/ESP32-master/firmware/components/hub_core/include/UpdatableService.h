#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <memory>
#include <chrono>
#include <stack>

namespace core {
namespace upd_srv {

enum class ClbkRet { Completed, Continue };
using StringType = std::string;
using Uuid = std::uint32_t;
using UpdClbk = std::function<ClbkRet()>;

static constexpr Uuid BadUuid = 0;

class UpdContext;  // Forward declaration

class IUpdatable {
public:
    using SPtr = std::shared_ptr<IUpdatable>;
    using ClockType = std::chrono::steady_clock;
    using TimePoint = ClockType::time_point;
    using Duration = ClockType::duration;
    using ClbkType = std::function<void()>;

    enum class UpdatableState : int { Update, Delay };

    virtual ClbkRet update( UpdContext& ctx );

    virtual ~IUpdatable() = default;

    template <typename DelayType>
    ClbkRet setDelay( const DelayType& delay, ClbkType delayClbk = nullptr ) {
        m_updateState = UpdatableState::Delay;
        m_timepoint = ClockType::now();
        m_delay = delay;
        m_delayClbk = delayClbk;
        return ClbkRet::Continue;
    }

    inline auto updUuid() const {
        return m_uuid;
    }

    inline void setUpdUuid( const Uuid uuid ) {
        m_uuid = uuid;
    }

    inline const auto& updName() const {
        return m_name;
    }

    inline void setUpdName( StringType&& name ) {
        m_name = std::move( name );
    }

    virtual ClbkRet updateComponent( UpdContext& ctx ) = 0;

private:
    Uuid m_uuid = BadUuid;
    StringType m_name;

protected:
    UpdatableState m_updateState = UpdatableState::Update;
    TimePoint m_timepoint;
    Duration m_delay;
    ClbkType m_delayClbk;
};  // class IUpdatable

class UpdContext {
    using ClockType = IUpdatable::ClockType;
    using Duration = ClockType::duration;

    IUpdatable::TimePoint tp;

public:
    UpdContext()
    : tp{ ClockType::now() } {}

    inline void tryYield() {
        if( isYield() ) {
            yield();
        }
    }

    void yield();

    void sleep_for( const Duration& delay );

    bool isYield() const;
};

Uuid add( UpdClbk&& clbk, StringType&& name = StringType() );

Uuid add( IUpdatable::SPtr&& updatable, StringType&& name = StringType() );

void remove( const Uuid uuid );

void update( UpdContext& ctx );

bool isEmpty();

void loop();

}  // namespace upd_srv
}  // namespace core
