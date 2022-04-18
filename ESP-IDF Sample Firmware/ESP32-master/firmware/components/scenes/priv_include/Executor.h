#pragma once

#include "ThenBlock.h"

#include <list>
#include <tuple>

namespace bus {
class IBusClient;
}  // namespace bus

namespace scenes {

template <typename ScenesManagerType, typename ActionInfoType>
class Executor : public ThenBlockConstVisitor {
public:
    static constexpr auto PostponedTimeoutSec = 20;

    using ItemsList = std::list<std::string>;

    using ValueItems = std::tuple<common::IValue::UPtr, ItemsList>;
    using ValueToItemsList = std::list<ValueItems>;

    Executor( ScenesManagerType& scenesManager )
    : m_scenesManager( scenesManager ){};

    void visit( const ThenBlockSetItemValue& block ) override;

    void visit( const ThenBlockSendHttpRequest& block ) override;

    void visit( const ThenBlockSetValueMulticast& block ) override;

    void visit( const ThenBlockSetValueMulticast2& block ) override;

    void visit( const ThenBlockSetValueMulticast3& block ) override;

    void visit( const ThenBlockSetDeviceArmed& block ) override;

#ifdef CONFIG_ADVANCED_SCENES
    void visit( const ThenBlockSwitchHouseMode& block ) override;

    void visit( const ThenBlockExecuteScene& block ) override;
#endif  // CONFIG_ADVANCED_SCENES

    void visit( const ThenBlockCloudCommand& block ) override;

    void execute( const ThenBlock& thenBlock,
                  ActionInfoType* info,
                  const TimePointType& prevTp,
                  const TimePointType& currTp );  // return true if thenBlock completed

    bool send( bus::IBusClient& busClient );

    void onCompleted( const StringType& id, bool isSuccess );

private:
    bool isDelayExpired( const ThenBlock& block );

    bool isDeviceOfItemReachable( const StringType& itemId ) const;

private:
    ScenesManagerType& m_scenesManager;
    ActionInfoType* m_actionsInfo = nullptr;
    TimePointType m_prevTp, m_currTp;
    ValueToItemsList m_valueToItems;

    std::map<StringType, ActionInfoType&> m_completedIds;
};

}  // end of namespace scenes

#include "../src/ExecutorImpl.hpp"
