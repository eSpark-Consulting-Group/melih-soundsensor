#pragma once

#include "Common.h"

#include <functional>

namespace scenes {

class ScenesDBIterator {
public:
    using StorType = storage::KeyValueStorage;
    using ItrType = StorType::ConstIterator;

    ScenesDBIterator( StorType& stor )
    : m_stor( stor ) {}

    virtual ~ScenesDBIterator() = default;

    virtual void OnScene( const StringType& storKey, const ItrType& itr, StorType& stor ){};

    virtual void OnConditionBlock( const StringType& storKey, const ItrType& itr, StorType& stor ){};

    virtual void OnActionBlock( const StringType& storKey, const ItrType& itr, StorType& stor ){};

    bool next();

    inline bool isEnd() const {
        return m_stor.cend() == itr;
    }

    inline void reset() {
        itr = m_stor.cbegin();
    }

private:
    StorType& m_stor;
    ItrType itr;
};

class ScenesDBClbkIterator : public ScenesDBIterator {
public:
    using ClbkType = std::function<void( const StringType&, const ItrType&, StorType& )>;

    using ScenesDBIterator::ScenesDBIterator;

    ScenesDBClbkIterator( StorType& stor, ClbkType sceneClbk, ClbkType condClbk, ClbkType actionClbk )
    : ScenesDBIterator( stor )
    , m_sceneClbk( sceneClbk )
    , m_condClbk( condClbk )
    , m_actionClbk( actionClbk ){};

    inline void setSceneClbk( ClbkType sceneClbk ) {
        m_sceneClbk = sceneClbk;
    }

    inline void setConditionClbk( ClbkType condClbk ) {
        m_condClbk = condClbk;
    }

    inline void setActionClbk( ClbkType actionClbk ) {
        m_actionClbk = actionClbk;
    }

    void OnScene( const StringType& storKey, const ItrType& itr, StorType& stor ) override {
        if( m_sceneClbk ) {
            m_sceneClbk( storKey, itr, stor );
        }
    };

    void OnConditionBlock( const StringType& storKey, const ItrType& itr, StorType& stor ) override {
        if( m_condClbk ) {
            m_condClbk( storKey, itr, stor );
        }
    };

    void OnActionBlock( const StringType& storKey, const ItrType& itr, StorType& stor ) override {
        if( m_actionClbk ) {
            m_actionClbk( storKey, itr, stor );
        }
    };

private:
    ClbkType m_sceneClbk, m_condClbk, m_actionClbk;
};

}  // namespace scenes
