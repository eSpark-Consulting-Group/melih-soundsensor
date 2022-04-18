#pragma once

#include "sdkconfig.h"

#include "IValue.h"
#include "IRamBufStream.h"
#include "KeyValueStorageFactory.h"
#include "Uuid.h"

#include <string>
#include <tuple>

namespace scenes {

using StringType = std::string;
using ValueType = common::IValue;
using ValueTypeUPtr = common::IValue::UPtr;
using BufferStreamType = common::IRamBufferStream;
using StorUPtr = storage::KeyValueStorageFactory::StoragePtr;
using StorType = storage::KeyValueStorage;
using BlobType = storage::KeyValueStorage::BlobType;
using KVSValue = storage::KeyValueStorage::ConstIterator::ValueType;
using Uint8Set = std::set<std::uint8_t>;
using ResultType = std::tuple<int, StringType>;

StorUPtr getLocalStorage();

StorUPtr getGlobalStorage();

class Uuid : public common::Uuid {
public:
    using common::Uuid::Uuid;

    Uuid( const common::Uuid& uuid )
    : common::Uuid( uuid ) {}

    Uuid( const std::string& str );

    inline const std::uint64_t& toUint64() const {
        return m_uuid.uuid64;
    }
};

using UuidList = common::UuidList;
using UuidSet = common::UuidSet;

template <typename SceneType>
using ScenesMap = std::map<Uuid, typename SceneType::UPtr>;

extern const char LOCAL_SCENES_PATH[];
extern const char GLOBAL_SCENES_PATH[];
extern const char SCENES_CACHE_PATH[];

}  // end of namespace scenes
