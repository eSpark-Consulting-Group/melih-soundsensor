#pragma once

#include "IReadStream.h"
#include "IWriteStream.h"

namespace scenes {

class ScenesInfo {
public:
    enum class State {
        Active,
        Completed,
        Failed,
        RecentlyAdded,
        AdditionInitialized,
        RecentlyEdited,
        EditionInitialized
    };

    ScenesInfo( const State state = State::Active )
    : m_state( state ){};

    inline auto state() const {
        return m_state;
    }

    inline void setState( State newState ) {
        m_state = newState;
    }

    inline bool write( common::IWriteStream& stream ) const {
        return stream.write( m_currActionIndex ) && stream.write( m_state );
    }

    inline bool read( common::IReadStream& stream ) {
        return stream.read( m_currActionIndex ) && stream.read( m_state );
    }

    inline auto actionIndex() const {
        return m_currActionIndex;
    }

    inline void setActionIndex( unsigned int actionIndex ) {
        m_currActionIndex = actionIndex;
    }

    inline bool isActive() const {
        return State::Active == m_state;
    }

    inline bool isCompleted() const {
        return State::Completed == m_state;
    }

    inline void setCompleted() {
        m_state = State::Completed;
    }

    inline bool isFailed() const {
        return State::Failed == m_state;
    }

    inline void setFailed() {
        m_state = State::Failed;
    }

private:
    unsigned int m_currActionIndex = 0;
    State m_state;
};  // class ScenesInfo

}  // namespace scenes
