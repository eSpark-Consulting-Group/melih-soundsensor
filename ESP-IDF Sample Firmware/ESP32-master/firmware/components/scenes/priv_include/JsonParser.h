#pragma once

#include "Common.h"
#include "Scene.h"

#include <tuple>

namespace gason {
class JsonValue;
}  // end of namespace gason

namespace scenes {

template <typename SceneType>
std::tuple<int, std::string, typename SceneType::UPtr> parseScene( gason::JsonValue& jsonRoot,
                                                                   const Uuid& scenesUuid,
                                                                   storage::KeyValueStorage& stor );

using storKeyClbk = StringType ( * )( const Uuid& );

std::tuple<int, std::string> saveSceneToDbAndFile( const StringType& scenesPath,
                                                   gason::JsonValue& jsonRoot,
                                                   const Uuid& sceneUuid,
                                                   storKeyClbk storKeyClbk );

std::tuple<int, std::string, UuidSet> parseWhenBlocksArray( const Uuid& sceneUuid,
                                                            const gason::JsonValue& jsonRoot,
                                                            storage::KeyValueStorage& stor,
                                                            const WhenBlock::ParentType parentType );

class ActionsParser {
public:
    using ParseResult = std::tuple<int, std::string, Uuid>;

    ActionsParser( const Uuid& _sceneUuid, storage::KeyValueStorage& _stor )
    : m_sceneUuid( _sceneUuid )
    , m_stor( _stor ) {}

    std::tuple<int, std::string, UuidList> parse( gason::JsonValue& jsonRoot );

    ParseResult parseThenBlock( gason::JsonValue& jsonRoot );

    ParseResult parseCloudCommandThenBlock( gason::JsonValue& fields );

#ifdef CONFIG_ADVANCED_SCENES
    ParseResult parseRunSceneThenBlock( gason::JsonValue& fields );

    ParseResult parseSwitchHouseModeThenBlock( gason::JsonValue& fields );
#endif  // CONFIG_ADVANCED_SCENES

    ParseResult parseSetDeviceArmedThenBlock( gason::JsonValue& fields );

    ParseResult parseSendHttpRequestThenBlock( gason::JsonValue& fields );

    ParseResult parseSetItemValueThenBlock( gason::JsonValue& fields );

    bool trySaveThenBlockSetItemValue();

private:
    const Uuid& m_sceneUuid;
    storage::KeyValueStorage& m_stor;
    DelayTypeSec m_delay{ 0 };
    ThenBlockSetValueMulticast3::UPtr m_setVal;
};

}  // end of namespace scenes
