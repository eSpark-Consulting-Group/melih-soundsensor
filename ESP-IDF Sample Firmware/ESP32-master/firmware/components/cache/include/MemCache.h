#pragma once

#include <IValue.h>

#include <string>
#include <map>
#include <mutex>

namespace cache {

class MemCache {
public:
    using KeyT = std::string;
    using ValueT = common::IValue::UPtr;
    using MapT = std::map<KeyT, ValueT>;

    static MemCache& get();

    void setValueByKey( const std::string& key, common::IValue::UPtr&& value );

    ValueT getValueByKey( const KeyT& key );

    void remove( const KeyT& key );

protected:
    MemCache() = default;

protected:
    std::mutex m_mutex;
    MapT m_values;
};

};  // namespace cache
