#include "ThenBlock.h"
#include "conditions/Conditions.h"
#include "CommonResources.h"
#include "JsonStreamGenerator.h"

#include <functional>
#include <string>
#include <sstream>
#include <cstdio>

#include "FileStream.h"
#include "Storage.h"

#include "DebugUtils.h"

namespace scenes {

using namespace common::resources;
using namespace std::chrono_literals;

template <typename SceneType>
ResultType jsonifyScene( const StringType& scenesPath,
                         storage::KeyValueStorage& stor,
                         const StringType& sceneStorId,
                         common::JsonStreamGenerator& gen,
                         bool isOnlyId ) {
    auto scene = SceneType::create( sceneStorId, stor );
    if( !scene ) {
        gen.writeEmptyObject() && gen.writeItemDelimiter();
        return std::make_tuple( errors::ApplicationError, "Can't open scene from storage!" );
    }

    constexpr auto isNotify = true;
    if( !jsonify( scenesPath, *scene, gen, isNotify, isOnlyId ) ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    return std::make_tuple( errors::NotError, "" );
}

template <typename SceneType>
ResultType jsonifyAllScenes( const StringType& scenesPath,
                             storage::KeyValueStorage& stor,
                             common::JsonStreamGenerator& gen,
                             bool isOnlyId ) {
    if( !gen.writeOpeningArray() ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    int errorCode = errors::NotError;
    StringType errorDescr;

    auto onFileClbk = [&errorCode, &errorDescr, &gen, &stor, &scenesPath, isOnlyId]( const char* fname,
                                                                                     const char* fullPath ) {
        std::tie( errorCode, errorDescr ) = jsonifyScene<SceneType>( scenesPath, stor, fname, gen, isOnlyId );
        return errors::NotError == errorCode ? storage::Storage::WalkDir::Continue :
                                               storage::Storage::WalkDir::Complete;
    };
    storage::Storage::get().walkDir( scenesPath, onFileClbk, nullptr );

    if( errors::NotError == errorCode ) {
        if( gen.writeClosingSymbol() ) {
            return std::make_tuple( errors::NotError, "" );
        }
        else {
            return std::make_tuple( errors::GenJsonFailed, StringType{} );
        }
    }

    if( errors::GenJsonFailed != errorCode && !gen.writeClosingSymbol() ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    return std::make_tuple( errorCode, std::move( errorDescr ) );
}

class CondTimeCalc : public WhenBlockConstVisitor {
public:
    using SceneActionData = std::tuple<StringType, TimePointType>;  // MethodName, ActionTimepoint

    CondTimeCalc( const TimePointType& currTime, const ConditionsMap& uuidToCond )
    : m_currTime( currTime )
    , m_uuidToCond( uuidToCond ) {}

    std::optional<SceneActionData> calculateActionTime( const WhenBlock& block ) {
        m_actionTime = TimePointType();
        block.accept( *this );
        if( TimePointType() == m_actionTime ) {
            return std::nullopt;
        }

        auto ret = std::make_tuple( m_methodName, m_actionTime );
        return std::make_optional( std::move( ret ) );
    }

private:
    void tryUpdateActionTime( const TimePointType& nextTp, const StringType methodName ) {
        if( TimePointType() != nextTp && ( nextTp < m_actionTime || TimePointType() == m_actionTime ) ) {
            m_actionTime = nextTp;
            m_methodName = methodName;
        }
    }

    void visit( const WhenBlockInterval& block ) {
        const auto& creatTime = block.creationTime();
        const auto& periodSec = block.period();
        auto interval = toSeconds( m_currTime - creatTime );

        int count = ( interval / periodSec + 1 );
        TimePointType nextTp = creatTime + periodSec * count;
        tryUpdateActionTime( nextTp, method_names::IsInterval );
    };

    void visit( const WhenBlockIsDate& block ) {
        const auto& hours = block.hours();
        if( hours.empty() ) {
            return;
        }

        // Find proper day
        auto actionTime = m_currTime;
        if( block.hasDays() || block.hasWeekdays() ) {
            while( !block.isDayOfMonth( actionTime ) || !block.isDayOfWeek( actionTime ) ) {
                actionTime += 24h;
            }
        }

        // Set actionTime to the beginning of the day
        const TimeOfDay currTd( toTimeInfo( m_currTime ) );
        actionTime -= DelayTypeSec( currTd.valueSeconds() );

        // Find proper hour
        while( true ) {
            for( const auto& td : hours ) {
                auto nextActionTime = actionTime + DelayTypeSec( td.valueSeconds() );
                if( nextActionTime < m_currTime ) {
                    continue;
                }

                tryUpdateActionTime( nextActionTime, method_names::IsDate );
                return;
            }

            actionTime += 24h;
        }
    };

    void visit( const WhenBlockIsSunState& block ) {
        // Find proper day
        auto actionTime = m_currTime;
        if( block.hasDays() || block.hasWeekdays() ) {
            while( !block.isDayOfMonth( actionTime ) || !block.isDayOfWeek( actionTime ) ) {
                actionTime += 24h;
            }
        }

        // Set actionTime to the beginning of the day
        const TimeOfDay currTd( toTimeInfo( m_currTime ) );
        actionTime -= DelayTypeSec( currTd.valueSeconds() );

        // Find proper hour
        while( true ) {
            const auto timeOfDay = block.trigTime( toTimeInfo( actionTime ) );
            auto nextActionTime = actionTime + DelayTypeSec( timeOfDay.valueSeconds() );
            if( nextActionTime > m_currTime ) {
                tryUpdateActionTime( nextActionTime, method_names::IsSunState );
                return;
            }

            actionTime += 24h;
        }
    };

    void visit( const WhenBlockIsOnce& block ) {
        auto td = block.dayTime();
        TimeInfo t = {};
        t.tm_year = block.year() - 1900;
        t.tm_mon = block.month() - 1;
        t.tm_mday = block.day();
        std::tie( t.tm_hour, t.tm_min, t.tm_sec ) = td.hms();
        auto nextTp = TimePointType( DelayTypeSec( mktime( &t ) ) );
        if( nextTp > m_currTime ) {
            tryUpdateActionTime( nextTp, method_names::IsOnce );
        }
    };

    void visit( const WhenBlockOr& block ) {
        const auto prevMethodName = m_methodName;
        const auto prevActionTime = m_actionTime;

        m_actionTime = TimePointType();
        const auto& conditions = block.conditions();
        for( const auto& condUuid : conditions ) {
            const auto itr = m_uuidToCond.find( condUuid );
            if( m_uuidToCond.end() == itr || nullptr == itr->second ) {
                continue;
            }
            const auto& cond = *( itr->second );
            m_actionTime = TimePointType();
            auto nextData = calculateActionTime( cond );
            if( !nextData ) {
                continue;
            }
            auto [methodName, nextTp] = *nextData;
            tryUpdateActionTime( nextTp, methodName );
        }

        const auto newMethodName = m_methodName;
        const auto newActionTime = m_actionTime;
        m_methodName = prevMethodName;
        m_actionTime = prevActionTime;
        tryUpdateActionTime( newActionTime, newMethodName );
    }

    void visit( const WhenBlockAnd& block ) {
        FUNC_LOGE( "Method \"hub.scenes.time.list\" can't work with \"and\" blocks in scenes" );
    }

private:
    const TimePointType m_currTime;
    const ConditionsMap& m_uuidToCond;
    StringType m_methodName;
    TimePointType m_actionTime;
};  // class TimeListJsonificator

inline StringType getExecDateStr( const TimePointType& actionTime ) {
    char buff[32];
    const auto& lt = toTimeInfo( actionTime );
    std::strftime( buff, sizeof( buff ), "%FT%T%z", &lt );
    return buff;
}

template <typename SceneType>
ResultType jsonifyTimeList( const ScenesMap<SceneType>& scenesMap,
                            const ConditionsMap& conditionsMap,
                            common::JsonStreamGenerator& gen ) {
    CondTimeCalc tlVis( currentTime(), conditionsMap );
    using SceneActionData = std::tuple<Uuid, StringType, TimePointType>;
    std::list<SceneActionData> scenesActionsList;
    for( const auto& [conditionUuid, conditionUPtr] : conditionsMap ) {
        if( WhenBlock::ParentType::Scene != conditionUPtr->parentType() ) {
            continue;
        }
        auto actionTime = tlVis.calculateActionTime( *conditionUPtr );
        if( actionTime ) {
            auto [methodName, actionTimepoint] = *actionTime;
            scenesActionsList.emplace_back( conditionUPtr->parentUuid(), methodName, actionTimepoint );
        }
    }
    scenesActionsList.sort( []( const SceneActionData& d1, const SceneActionData& d2 ) {
        return std::get<2>( d1 ) < std::get<2>( d2 );  // 2 is action timepoint
    } );

    if( !gen.writePropertyKey( tokens::TimeScenes ) || !gen.writeOpeningArray() ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    for( const auto& [sceneId, methodName, actionTp] : scenesActionsList ) {
        const auto& scene = scenesMap.find( sceneId )->second;
        if( !scene ) {
            continue;
        }

        if( !gen.writeOpeningObject() || !gen.writeProperty( tokens::SceneId, scene->externStorId() ) ||
            !gen.writeProperty( tokens::SceneName2, scene->name() ) || !gen.writeProperty( "methodName", methodName ) ||
            !gen.writeProperty( "executionDate", getExecDateStr( actionTp ) ) || !gen.writeClosingSymbol() ) {
            return std::make_tuple( errors::GenJsonFailed, StringType{} );
        }
    }

    return gen.writeClosingSymbol() ? std::make_tuple( errors::NotError, StringType() ) :
                                      std::make_tuple( errors::GenJsonFailed, StringType{} );
}

}  // end of namespace scenes
