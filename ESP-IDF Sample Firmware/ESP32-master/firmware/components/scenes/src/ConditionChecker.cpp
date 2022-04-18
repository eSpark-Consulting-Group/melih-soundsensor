#include "ConditionChecker.h"

#include "Time.h"
#include "CommonResources.h"

#include "Cache.h"
#include "IValueCast.h"
#include "values/BoolValue.h"
#include "values/TableValue.h"
#include "values/TokenValue.h"
#include "values/IntValue.h"

#include "DeviceInfo.h"
#include "dev_dis/DevDiscovery.h"

#include <algorithm>

#include "DebugUtils.h"

namespace scenes {

using namespace common::resources;

namespace {

constexpr auto MAX_PASSES_COUNT = 5;

bool compareValues( const WhenBlockIsItemState& block, const common::IValue& currValue ) {
    if( WhenBlockIsItemState::BlockType::IsButtonState == block.blockType() ) {
        auto tbl = common::iValueCast<common::TableValue>( &currValue );
        if( !tbl ) {
            return false;
        }
        const auto* tblVal = ( *tbl )[values::Button_state];
        return tblVal && ( block.value() == *tblVal || *tblVal == block.value() );
    }

    switch( block.compareMethod() ) {
    case WhenBlockIsItemState::CompareMethod::Equal:
        // Need double equality check because equality operator is binary
        // (this is like difference between equality and identity)
        return block.value() == currValue || currValue == block.value();
    case WhenBlockIsItemState::CompareMethod::More:
        return currValue > block.value();
    case WhenBlockIsItemState::CompareMethod::Less:
        return currValue < block.value();
    case WhenBlockIsItemState::CompareMethod::MoreOrEqual:
        return currValue >= block.value();
    case WhenBlockIsItemState::CompareMethod::LessOrEqual:
        return currValue <= block.value();
    default:
        FUNC_LOGE( "Bad compare method %d!", (int)block.compareMethod() );
        return false;
    }
}

inline bool compareCachedValues( const WhenBlockIsItemState& block ) {
    auto cachedValue = cache::Cache::get().getValueByKey( block.itemId() );
    if( !cachedValue ) {
        return false;
    }
    return cachedValue && compareValues( block, *cachedValue );
}

bool checkArmedState( const WhenBlockIsItemState& block ) {
    if( WhenBlockIsItemState::None == block.armedState() ) {
        return true;
    }

    auto deviceInfo = gateways::getDeviceInfoByItemId( block.itemId() );
    if( !deviceInfo ) {
        return false;
    }

    bool isDeviceArmed = deviceInfo->isArmed();
    return ( isDeviceArmed && WhenBlockIsItemState::IsArmed == block.armedState() ) ||
           ( !isDeviceArmed && WhenBlockIsItemState::IsDisarmed == block.armedState() );
}

}  // namespace

BaseConditionChecker::BaseConditionChecker( UuidSet& prevTrig, UuidSet& currTrig )
: m_prevTrig( prevTrig )
, m_currTrig( currTrig ) {}

bool BaseConditionChecker::isTriggered( const WhenBlock& block ) {
    if( m_currTrig.end() != m_currTrig.find( block.uuid() ) ) {
        return false;
    }

    m_isTriggered = false;
    block.accept( *this );
    if( !m_isTriggered ) {
        return false;
    }

    m_currTrig.emplace( block.uuid() );

    if( m_prevTrig.end() != m_prevTrig.find( block.uuid() ) ) {
        return false;
    }

    return WhenBlock::ParentType::Scene == block.parentType();
}

void LogicConditionChecker::onPassCompleted() {
    m_passesCount += 1;
}

bool LogicConditionChecker::isCompleted() {
    return m_passesCount >= MAX_PASSES_COUNT;
}

void LogicConditionChecker::update() {
    m_passesCount = 0;
}

#ifdef CONFIG_ADVANCED_SCENES
void LogicConditionChecker::visit( const WhenBlockAnd& block ) {
    for( const auto uuid : block.conditions() ) {
        if( m_currTrig.end() == m_currTrig.find( uuid ) ) {
            return;  // m_isTriggered == false by default
        }
    }

    m_isTriggered = true;
}

void LogicConditionChecker::visit( const WhenBlockOr& block ) {
    if( m_currTrig.empty() ) {
        return;  // m_isTriggered == false by default
    }

    for( const auto uuid : block.conditions() ) {
        if( m_currTrig.end() != m_currTrig.find( uuid ) ) {
            m_isTriggered = true;
            return;
        }
    }

    // m_isTriggered == false by default
}

void LogicConditionChecker::visit( const WhenBlockNot& block ) {
    if( m_currTrig.end() == m_currTrig.find( block.condition() ) ) {
        m_isTriggered = true;
    }
}
#endif  // CONFIG_ADVANCED_SCENES

void LogicConditionChecker::visit( const WhenBlockEqual& block ) {
    const auto& conditions = block.conditions();
    if( conditions.empty() ) {
        return;  // m_isTriggered == false by default
    }

    auto end = conditions.end();
    auto itr = conditions.begin();

    bool isTrue = m_currTrig.end() != m_currTrig.find( *itr );

    while( ( ++itr ) != end ) {
        if( ( isTrue && m_currTrig.end() == m_currTrig.find( *itr ) ) ||
            ( !isTrue && m_currTrig.end() != m_currTrig.find( *itr ) ) ) {
            return;  // m_isTriggered == false by default
        }
    }

    m_isTriggered = true;
}

bool TimeConditionChecker::isTriggered( const WhenBlock& block ) {
    if( m_currTrig.end() != m_currTrig.find( block.uuid() ) ) {
        return false;
    }

    m_isTriggered = false;
    block.accept( *this );
    if( !m_isTriggered ) {
        return false;
    }

    m_currTrig.emplace( block.uuid() );

    return WhenBlock::ParentType::Scene == block.parentType();
}

void TimeConditionChecker::visit( const WhenBlockInterval& block ) {
    const auto& periodSec = block.period();
    const auto& periodMsec = toMillis( periodSec );

    const auto& creatTime = block.creationTime();
    if( creatTime > m_currTp || creatTime > m_prevTp ) {
        return;  // m_isTriggered == false by default
    }

    auto execPeriod = toMillis( m_currTp - creatTime );
    auto currPeriodIdx = execPeriod / periodMsec;

    auto prevPeriod = toMillis( m_prevTp - creatTime );
    auto prevPeriodIdx = prevPeriod / periodMsec;

    m_isTriggered = currPeriodIdx > prevPeriodIdx;
};

void TimeConditionChecker::visit( const WhenBlockIsDate& block ) {
    const auto prevInfo = toTimeInfo( m_prevTp );
    const auto currInfo = toTimeInfo( m_currTp );
    const TimeOfDay prevTimeOfDay( prevInfo ), currTimeOfDay( currInfo );

    for( const auto& td : block.hours() ) {
        if( prevTimeOfDay < td && currTimeOfDay >= td ) {
            m_isTriggered = checkDateBase( block );
            break;
        }
        else if( currTimeOfDay < td ) {
            break;  // m_isTriggered == false by default
        }
    }
};

void TimeConditionChecker::visit( const WhenBlockIsDateRange& block ) {
    const auto currInfo = toTimeInfo( m_currTp );

    // m_isTriggered == false by default there

    const auto currYear = getYear( currInfo );
    if( block.startYear() && block.startYear().operator unsigned int() > currYear ) {
        return;
    }
    if( block.endYear() && block.endYear().operator unsigned int() < currYear ) {
        return;
    }

    const auto currMonth = getMonth( currInfo );
    if( block.startMonth() && block.startMonth().operator unsigned int() > currMonth ) {
        return;
    }
    if( block.endMonth() && block.endMonth().operator unsigned int() < currMonth ) {
        return;
    }

    const auto currDay = getDayOfMonth( currInfo );
    if( block.startDay() && block.startDay().operator unsigned int() > currDay ) {
        return;
    }
    if( block.endDay() && block.endDay().operator unsigned int() < currDay ) {
        return;
    }

    const TimeOfDay currTimeOfDay{ currInfo };
    if( block.startTime() && block.startTime() > currTimeOfDay ) {
        return;
    }
    if( block.endTime() && block.endTime() < currTimeOfDay ) {
        return;
    }

    m_isTriggered = true;
};

void TimeConditionChecker::visit( const WhenBlockIsSunState& block ) {
    auto prevInfo = toTimeInfo( m_prevTp );
    auto currInfo = toTimeInfo( m_currTp );
    const TimeOfDay prevTimeOfDay( prevInfo ), currTimeOfDay( currInfo );

    auto td = block.trigTime( currInfo );
    if( !( prevTimeOfDay < td && currTimeOfDay >= td ) ) {
        return;  // m_isTriggered == false by default
    }

    m_isTriggered = checkDateBase( block );
};

void TimeConditionChecker::visit( const WhenBlockIsOnce& block ) {
    auto prevInfo = toTimeInfo( m_prevTp );
    auto currInfo = toTimeInfo( m_currTp );
    const TimeOfDay prevTimeOfDay( prevInfo ), currTimeOfDay( currInfo );
    auto td = block.dayTime();
    const auto currMDay = getDayOfMonth( currInfo );
    const auto currMonth = getMonth( currInfo );
    const auto currYear = getYear( currInfo );

    m_isTriggered = ( prevTimeOfDay < td && currTimeOfDay >= td ) && currMDay == block.day() &&
                    currMonth == block.month() && currYear == block.year();
};

bool TimeConditionChecker::checkDateBase( const WhenBlockIsDateBase& block ) const {
    return block.isDayOfMonth( m_currTp ) && block.isDayOfWeek( m_currTp );
}

void TimeConditionChecker::update( const TimePointType& currTp ) {
    m_prevTp = m_currTp;
    m_currTp = currTp;
}

void ItemsConditionChecker::onItemUpdated( const StringType& itemId, ValueTypeUPtr&& value ) {
    auto itr = m_valuesStacks.find( itemId );
    if( m_valuesStacks.end() != itr ) {
        itr->second.emplace( std::move( value ) );
    }
}

void ItemsConditionChecker::onDeviceUpdated( const StringType& deviceId, const StringType& key, ValueTypeUPtr value ) {
    if( key != tokens::Armed ) {
        return;
    }

    auto itr = m_devicesStacks.find( deviceId );
    if( m_devicesStacks.end() != itr ) {
        itr->second.emplace( std::move( value ) );
    }
}

void ItemsConditionChecker::update() {
    m_values.clear();
    for( auto& pair : m_valuesStacks ) {
        if( !pair.second.empty() ) {
            m_values[pair.first] = std::move( pair.second.top() );
            pair.second.pop();
        }
    }

    m_devices.clear();
    for( auto& pair : m_devicesStacks ) {
        if( !pair.second.empty() ) {
            m_devices[pair.first] = std::move( pair.second.top() );
            pair.second.pop();
        }
    }
}

void ItemsConditionChecker::visit( const WhenBlockIsItemState& block ) {
    if( checkItem( block ) || checkDevice( block ) ) {
        m_isTriggered = true;
    }
};

void ItemsConditionChecker::visit( const WhenBlockIsUserLockOperation& block ) {
    common::TableValue::UPtr itemUpdateValue;
    auto itr = m_values.find( block.itemId() );
    if( m_values.end() != itr ) {
        itemUpdateValue = common::iValueCast<common::TableValue>( std::move( itr->second ) );
    }
    if( !itemUpdateValue ) {
        return;
    }

    const auto action = common::iValueCast<common::StringValue>( ( *itemUpdateValue )[tokens::Action] );
    if( !action || block.action() != action->get() ) {
        return;
    }

    const auto userId = common::iValueCast<common::IntValue>( ( *itemUpdateValue )[tokens::UserId] );
    if( !userId ) {
        return;
    }

    m_isTriggered = ( block.userCodeId() == userId->get() );
}

void ItemsConditionChecker::visit( const WhenBlockHasAtLeastOneDictionaryValue& block ) {
    auto itr = m_values.find( block.itemId() );
    if( m_values.end() != itr ) {
        m_isTriggered = ( block.value() == itr->second );
    }
}

bool ItemsConditionChecker::checkItem( const WhenBlockIsItemState& block ) {
    auto itr = m_values.find( block.itemId() );
    if( m_values.end() == itr ) {
        return compareCachedValues( block ) && checkArmedState( block );
    }

    return itr->second && compareValues( block, *( itr->second ) ) && checkArmedState( block );
}

bool ItemsConditionChecker::checkDevice( const WhenBlockIsItemState& block ) {
    const auto deviceId = gateways::getDeviceIdByItemId( block.itemId() );
    const auto itr = m_devices.find( deviceId );
    if( m_devices.end() == itr ) {
        return checkArmedState( block ) && compareCachedValues( block );
    }

    const auto boolVal = common::iValueCast<common::BoolValue>( itr->second.get() );
    if( !boolVal ) {
        return false;
    }
    bool isDeviceArmed = boolVal->get();
    if( ( isDeviceArmed && WhenBlockIsItemState::IsArmed != block.armedState() ) ||
        ( !isDeviceArmed && WhenBlockIsItemState::IsDisarmed != block.armedState() ) ) {
        return false;
    }

    return compareCachedValues( block );
}

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
void WifiBeaconConditionChecker::onDevicesDiscovered( const dev_dis::UuidToState& registered,
                                                      const dev_dis::AddrToState& unknow ) {
    // Don't use this log in production code! For debug only!
    /*if( !registered.empty() ) {
        FUNC_LOGI( "_________________ Founded/Changed registered devices _________________" );
        for( const auto& [uuid, state] : registered ) {
            const auto& [rssi, tp] = state;
            FUNC_LOGI( "-----> addr=%s, rssi=%d", uuid.toHexString().c_str(), int( rssi ) );
        }
        FUNC_LOGI( "-------------------------------------------------------------------\n" );
    }*/

    m_registeredNew.insert( registered.begin(), registered.end() );
    m_unknowNew.insert( unknow.begin(), unknow.end() );
}

void WifiBeaconConditionChecker::update() {
    m_registered = std::move( m_registeredNew );
    m_unknow = std::move( m_unknowNew );
}

void WifiBeaconConditionChecker::visit( const WhenBlockIsUnknowDeviceDetected& block ) {
    m_isTriggered = !m_unknow.empty();
}

void WifiBeaconConditionChecker::visit( const WhenBlockIsDeviceDetected& block ) {
    auto& devices = m_blockUuidToDevices[block.uuid()];

    if( !m_registered.empty() ) {
        for( const auto& uuidStr : block.devicesIds() ) {
            const auto devUuid = Uuid::createFromHexStr( uuidStr );
            if( !devUuid ) {
                continue;
            }
            const auto itr = m_registered.find( *devUuid );
            if( m_registered.end() == itr ) {
                continue;
            }

            const auto& [rssi, timepoint] = itr->second;
            if( block.checkRssi( rssi ) ) {
                devices.insert( *devUuid );
            }
            else {
                devices.erase( *devUuid );
            }
        }
    }

    m_isTriggered = !devices.empty();
}
#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

void ConditionInitor::visit( const WhenBlockIsItemState& block ) {
    m_checker.addItemId( block.itemId() );

    if( WhenBlockIsItemState::ArmedState::None != block.armedState() ) {
        auto deviceInfo = gateways::getDeviceInfoByItemId( block.itemId() );
        if( deviceInfo ) {
            m_checker.addDeviceId( deviceInfo->getId() );
        }
    }
}

void ConditionInitor::visit( const WhenBlockIsUserLockOperation& block ) {
    m_checker.addItemId( block.itemId() );
}

void ConditionInitor::visit( const WhenBlockHasAtLeastOneDictionaryValue& block ) {
    m_checker.addItemId( block.itemId() );
}

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
void ConditionInitor::visit( const WhenBlockIsUnknowDeviceDetected& block ) {
    auto instance = dev_dis::DevDiscovery::get();
    if( instance ) {
        instance->startUnknowDevDetection();
    }
}
#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

void RecursiveConditionInitor::initConditions( const UuidSet& conditions ) {
    for( const auto id : conditions ) {
        auto whenBlock = WhenBlock::create( id, m_stor );
        if( whenBlock ) {
            init( *whenBlock );
        }
    }
}

void RecursiveConditionInitor::visit( const WhenBlockNot& block ) {
    auto whenBlock = WhenBlock::create( block.condition(), m_stor );
    if( whenBlock ) {
        init( *whenBlock );
    }
}

}  // end of namespace scenes
