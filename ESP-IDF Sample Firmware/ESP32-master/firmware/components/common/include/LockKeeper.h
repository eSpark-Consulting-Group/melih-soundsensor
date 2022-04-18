#pragma once

#include <Mutex.h>

namespace common {

template <class LockPrimitive = common::Mutex, class Timeout = common::Mutex::Timeout>
class LockKeeper {
public:
    explicit LockKeeper( LockPrimitive& lock, Timeout timeout )
    : m_lock( lock ) {
        m_isLocked = m_lock.take( timeout );
    }

    ~LockKeeper() {
        release();
    }

    LockKeeper( const LockKeeper& other ) = delete;
    LockKeeper( const LockKeeper&& other ) = delete;
    LockKeeper& operator=( const LockKeeper& other ) = delete;

    inline bool isLocked() const {
        return m_isLocked;
    }

    inline bool release() {
        if( m_isLocked ) {
            m_isLocked = !m_lock.release();
        }

        return !m_isLocked;
    }

private:
    LockPrimitive& m_lock;
    bool m_isLocked = false;
};

};  // namespace common
