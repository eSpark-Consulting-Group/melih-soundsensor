#pragma once

#include "Common.h"
#include "IReadStream.h"
#include "IWriteStream.h"

namespace scenes {

class ActionInfo {
public:
    enum class State { Activated, Postponed, Completed, Failed, WaitingForConfirmation };

    ActionInfo()
    : m_activationTime( currentTime() ) {}

    ActionInfo( const TimePointType& activationTime, const bool isIgnoreResult )
    : m_activationTime( activationTime )
    , m_isIgnoreResult( isIgnoreResult ) {}

    const auto& activationTime() const {
        return m_activationTime;
    }

    bool write( common::IWriteStream& stream ) const {
        return stream.write( m_activationTime ) && stream.write( m_state ) && stream.write( m_isIgnoreResult );
    }

    bool read( common::IReadStream& stream ) {
        if( !stream.read( m_activationTime ) || !stream.read( m_state ) ) {
            return false;
        }
        stream.read( m_isIgnoreResult );
        return true;
    }

    auto state() const {
        return m_state;
    }

    inline bool isCompleted() const {
        return State::Completed == m_state;
    }

    inline void setCompleted() {
        m_state = State::Completed;
    }

    bool isPostponed() const {
        return State::Postponed == m_state;
    }

    void setPostponed( const DelayTypeSec& delay ) {
        m_state = State::Postponed;
        m_activationTime += delay;
    }

    void setActivationTime( const TimePointType& tp ) {
        m_activationTime = tp;
    }

    inline void setFailed() {
        m_state = State::Failed;
    }

    inline bool isFailed() const {
        return State::Failed == m_state;
    }

    inline bool isWaitingForConfirmation() const {
        return State::WaitingForConfirmation == m_state;
    }

    inline void setWaitingForConfirmation( const DelayTypeSec& delay ) {
        m_state = State::WaitingForConfirmation;
        m_activationTime += delay;
    }

    inline auto isIgnoreResult() const {
        return m_isIgnoreResult;
    }

    inline void SetIsIgnoreResult( bool isIgnoreResult ) {
        m_isIgnoreResult = isIgnoreResult;
    }

private:
    TimePointType m_activationTime;
    State m_state = State::Activated;
    bool m_isIgnoreResult = true;
};  // class ActionInfo

}  // namespace scenes
