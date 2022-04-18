#include "ScenesDBIterator.h"
#include "LocalScene.h"
#include "WhenBlock.h"
#include "ThenBlock.h"

#include <cstring>

namespace scenes {

bool ScenesDBIterator::next() {
    if( isEnd() ) {
        return false;
    }

    auto storKey = *itr;

    if( 0 == std::strncmp( LocalScene::StorPrefix, storKey.c_str(), std::strlen( LocalScene::StorPrefix ) ) ) {
        OnScene( storKey, itr, m_stor );
    }
    else if( 0 == std::strncmp( WhenBlock::StorPrefix, storKey.c_str(), std::strlen( WhenBlock::StorPrefix ) ) ) {
        OnConditionBlock( storKey, itr, m_stor );
    }
    else if( 0 == std::strncmp( ThenBlock::StorPrefix, storKey.c_str(), std::strlen( ThenBlock::StorPrefix ) ) ) {
        OnActionBlock( storKey, itr, m_stor );
    }

    ++itr;

    return true;
}

}  // namespace scenes
