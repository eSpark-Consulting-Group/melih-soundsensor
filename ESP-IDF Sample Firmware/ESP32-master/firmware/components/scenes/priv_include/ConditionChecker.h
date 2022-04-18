#pragma once

#include "WhenBlock.h"
#include "conditions/Conditions.h"
#include "dev_dis/Commands.h"

#include <tuple>
#include <set>
#include <stack>

namespace gateways {
class ItemInfo;
}

namespace scenes {

class BaseConditionChecker : public WhenBlockConstVisitor {
public:
    BaseConditionChecker( UuidSet& prevTrig, UuidSet& currTrig );

    bool isTriggered( const WhenBlock& block );

protected:
    UuidSet &m_prevTrig, &m_currTrig;
    bool m_isTriggered;
};

class LogicConditionChecker : public BaseConditionChecker {
public:
    using BaseConditionChecker::BaseConditionChecker;

    void onPassCompleted();

    bool isCompleted();

    void update();

private:
#ifdef CONFIG_ADVANCED_SCENES
    void visit( const WhenBlockAnd& block ) override;

    void visit( const WhenBlockOr& block ) override;

    void visit( const WhenBlockNot& block ) override;
#endif  // CONFIG_ADVANCED_SCENES

    void visit( const WhenBlockEqual& block ) override;

private:
    int m_passesCount{ 0 };
};

class TimeConditionChecker : public BaseConditionChecker {
public:
    using DataMap = std::map<StringType, ValueTypeUPtr>;
    using DeviceDataMap = std::map<StringType, std::tuple<StringType, ValueTypeUPtr>>;

    using BaseConditionChecker::BaseConditionChecker;

    bool isTriggered( const WhenBlock& block );

    inline void setCurrentTime( const TimePointType& tp ) {
        m_prevTp = tp;
        m_currTp = tp;
    }

    void update( const TimePointType& currTp );

private:
    void visit( const WhenBlockInterval& block ) override;

    void visit( const WhenBlockIsDate& block ) override;

    void visit( const WhenBlockIsDateRange& block ) override;

    void visit( const WhenBlockIsSunState& block ) override;

    void visit( const WhenBlockIsOnce& block ) override;

    bool checkDateBase( const WhenBlockIsDateBase& block ) const;

private:
    TimePointType m_prevTp, m_currTp;
};

class ItemsConditionChecker : public BaseConditionChecker {
public:
    using StringToValueMap = std::map<StringType, ValueTypeUPtr>;
    using ValuesStack = std::stack<ValueTypeUPtr>;
    using StringToItemsStackMap = std::map<StringType, ValuesStack>;

    using BaseConditionChecker::BaseConditionChecker;

    void onItemUpdated( const StringType& itemId, ValueTypeUPtr&& value );

    void onDeviceUpdated( const StringType& deviceId, const StringType& key, ValueTypeUPtr value );

    void update();

    inline void addItemId( const StringType& itemId ) {
        m_valuesStacks.emplace( itemId, ValuesStack() );
    }

    inline void addDeviceId( const StringType& deviceId ) {
        m_valuesStacks.emplace( deviceId, ValuesStack() );
    }

private:
    void visit( const WhenBlockIsItemState& block ) override;

    void visit( const WhenBlockIsUserLockOperation& block ) override;

    void visit( const WhenBlockHasAtLeastOneDictionaryValue& block ) override;

    bool checkItem( const WhenBlockIsItemState& block );

    bool checkDevice( const WhenBlockIsItemState& block );

private:
    StringToItemsStackMap m_valuesStacks;
    StringToValueMap m_values;

    StringToItemsStackMap m_devicesStacks;
    StringToValueMap m_devices;
};

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
class WifiBeaconConditionChecker : public BaseConditionChecker {
public:
    using BaseConditionChecker::BaseConditionChecker;

    void onDevicesDiscovered( const dev_dis::UuidToState& registered, const dev_dis::AddrToState& unknow );

    void update();

private:
    void visit( const WhenBlockIsUnknowDeviceDetected& block ) override;

    void visit( const WhenBlockIsDeviceDetected& block ) override;

private:
    dev_dis::UuidToState m_registered, m_registeredNew;
    dev_dis::AddrToState m_unknow, m_unknowNew;

    std::map<Uuid, std::set<Uuid>> m_blockUuidToDevices;
};
#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

class ConditionInitor : public WhenBlockConstVisitor {
public:
    ConditionInitor( ItemsConditionChecker& checker )
    : m_checker( checker ) {}

    inline void init( const WhenBlock& block ) {
        block.accept( *this );
    }

private:
    void visit( const WhenBlockIsItemState& block ) override;

    void visit( const WhenBlockIsUserLockOperation& block ) override;

    void visit( const WhenBlockHasAtLeastOneDictionaryValue& block ) override;

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
    void visit( const WhenBlockIsUnknowDeviceDetected& block ) override;
#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

private:
    ItemsConditionChecker& m_checker;
};

class RecursiveConditionInitor : public ConditionInitor {
public:
    RecursiveConditionInitor( storage::KeyValueStorage& stor, ItemsConditionChecker& checker )
    : ConditionInitor( checker )
    , m_stor( stor ) {}

    inline void init( const WhenBlock& block ) {
        block.accept( *this );
    }

private:
    void visit( const WhenBlockAnd& block ) override {
        initConditions( block.conditions() );
    }

    void visit( const WhenBlockOr& block ) override {
        initConditions( block.conditions() );
    }

    void visit( const WhenBlockNot& block ) override;

    void initConditions( const UuidSet& conditions );

private:
    storage::KeyValueStorage& m_stor;
};

}  // end of namespace scenes
