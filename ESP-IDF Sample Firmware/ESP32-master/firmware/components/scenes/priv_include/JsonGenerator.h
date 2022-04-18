#pragma once

#include "Common.h"
#include "WhenBlock.h"
#include <tuple>

namespace scenes {

template <typename SceneType>
bool jsonify( const StringType& scenesPath,
              const SceneType& scene,
              common::JsonStreamGenerator& gen,
              bool notify,
              bool isOnlyId );

template <typename SceneType>
ResultType jsonifyScene( const StringType& scenesPath,
                         storage::KeyValueStorage& stor,
                         const StringType& sceneStorId,
                         common::JsonStreamGenerator& gen,
                         bool isOnlyId );

template <typename SceneType>
ResultType jsonifyAllScenes( const StringType& scenesPath,
                             storage::KeyValueStorage& stor,
                             common::JsonStreamGenerator& gen,
                             bool isOnlyId );

template <typename SceneType>
ResultType jsonifyTimeList( const ScenesMap<SceneType>& scenesMap,
                            const ConditionsMap& conditionsMap,
                            common::JsonStreamGenerator& gen );

}  // end of namespace scenes

#include "../src/JsonGeneratorImpl.hpp"
