#include "JsonParser.h"
#include "GlobalScene.h"
#include "JsonParser.h"
#include "CommonResources.h"
#include "Utils.h"
#include "DeviceInfo.h"
#include "Time.h"

#include "IValueCast.h"

#include "Storage.h"
#include "FileStream.h"
#include "JsonStreamGenerator.h"

#include "gason.hpp"

#include <cstring>
#include <memory>
#include <cstdlib>
#include <cstdio>
#include <limits>

#include "DebugUtils.h"

namespace scenes {

using namespace common::resources;

template <>
std::tuple<int, std::string, GlobalScene::UPtr> parseScene<GlobalScene>( gason::JsonValue& jsonRoot,
                                                                         const Uuid& sceneUuid,
                                                                         storage::KeyValueStorage& stor ) {
    if( !jsonRoot.isObject() ) {
        return std::make_tuple( errors::InvalidParams, "parseScene: jsonRoot isn't json object", nullptr );
    }

    StringType name;
    {
        auto cStrName = jsonRoot( tokens::Name ).toString();
        if( cStrName ) {
            name = cStrName;
        }
    }

    StringType tag;
    {
        auto cStrTag = jsonRoot( tokens::Tag ).toString();
        if( cStrTag ) {
            tag = cStrTag;
        }
    }

    GlobalScene::ExecPolicy execPolicy = GlobalScene::ExecPolicy::IgnoreActionResult;
    auto policyCStr = jsonRoot( tokens::ExecPolicyField ).toString();
    if( policyCStr ) {
        if( 0 ==
            std::strncmp( policyCStr, tokens::ExecPolicyCheckResult, std::strlen( tokens::ExecPolicyCheckResult ) ) ) {
            execPolicy = GlobalScene::ExecPolicy::CheckActionResult;
        }
        else if( 0 != std::strncmp( policyCStr,
                                    tokens::ExecPolicyIgnoreResult,
                                    std::strlen( tokens::ExecPolicyIgnoreResult ) ) ) {
            FUNC_LOGW( "Wrong \"exec_policy\" value: %s!", policyCStr );
        }
    }

    auto whenBlockJson = jsonRoot( tokens::When );
    int error;
    std::string errorDescr;
    UuidSet whenUuidList;
    std::tie( error, errorDescr, whenUuidList ) =
        parseWhenBlocksArray( sceneUuid, whenBlockJson, stor, WhenBlock::ParentType::Scene );
    if( errors::NotError != error ) {
        return std::make_tuple( error, std::move( errorDescr ), nullptr );
    }

    auto thenBlockJson = jsonRoot( tokens::Then );
    ActionsParser actionParser( sceneUuid, stor );
    UuidList thenUuidList;
    std::tie( error, errorDescr, thenUuidList ) = actionParser.parse( thenBlockJson );
    if( errors::NotError != error ) {
        return std::make_tuple( error, std::move( errorDescr ), nullptr );
    }

    bool isSuccess = false;
    bool isEnabled = jsonRoot( tokens::Enabled ).toBool( &isSuccess );
    if( !isSuccess ) {
        return std::make_tuple( errors::ParseError, "parseScene: Can't parse \"enabled\" token!", nullptr );
    }

    auto scene = std::make_unique<GlobalScene>( sceneUuid,
                                                std::move( name ),
                                                std::move( tag ),
                                                std::move( whenUuidList ),
                                                std::move( thenUuidList ),
                                                isEnabled,
                                                execPolicy );
    if( !scene ) {
        return std::make_tuple( errors::ApplicationError, "Can't create scene in heap!", nullptr );
    }

    auto sceneId = jsonRoot( tokens::SubId ).toString();
    if( sceneId ) {
        scene->setPersistentStorId( sceneId );
    }

    return std::make_tuple( errors::NotError, std::string(), std::move( scene ) );
}

}  // namespace scenes
