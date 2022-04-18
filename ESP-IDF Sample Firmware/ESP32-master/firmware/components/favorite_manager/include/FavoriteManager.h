#pragma once

#include <stdint.h>
//#include <GatewayDefs.h>
#include <CommonResources.h>

#include <list>
#include <map>
#include <array>
#include <string.h>
#include <memory>

#include <Utils.h>
#include <Bus.h>

namespace common {
class JsonStreamGenerator;
};  // namespace common

namespace core {

namespace FavoriteManager {

enum FavoriteType : uint8_t { Device = 0, Item, Rule, TypeEnd };

extern const char* favoriteTypes[TypeEnd];

constexpr static auto favoriteTypesCount = TypeEnd;

// Broadcasts data
using FavoriteTypeIdBroadcastList = std::list<std::string>;
using FavoriteTypesListBroadcast = std::map<std::string, FavoriteTypeIdBroadcastList>;

bool getFavoriteList( common::JsonStreamGenerator& result, const char* favoriteTypeName );
bool updateFavoriteId( const char* favoriteTypeName, const char* favoriteId, const bool isAdd );
void updateFavoriteIdBroadcast( bus::IBusClient& client );

FavoriteType getType( const char* favoriteTypeName );
};  // namespace FavoriteManager
}  // namespace core
