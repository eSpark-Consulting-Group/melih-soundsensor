#pragma once

#include "IValue.h"
#include <map>
#include <memory>
#include <string>

namespace common {

class TableValue final : public IValue {
public:
    using UPtr = std::unique_ptr<TableValue>;
    using ValPtr = IValue::UPtr;
    using Key = std::string;
    using ValMap = std::map<Key, ValPtr>;
    using Itr = ValMap::iterator;
    using ConstItr = ValMap::const_iterator;

public:
    TableValue() = default;

    void accept( IValueVisitorConst& visitor ) const override;
    void accept( IValueVisitor& visitor ) override;

    void insert( const Key& key, ValPtr&& value );

    ValPtr remove( const Key& key );

    inline Itr begin() {
        return m_value.begin();
    }

    inline Itr end() {
        return m_value.end();
    }

    inline ConstItr begin() const {
        return m_value.cbegin();
    }

    inline ConstItr end() const {
        return m_value.cend();
    }

    inline ConstItr cbegin() const {
        return m_value.cbegin();
    }

    inline ConstItr cend() const {
        return m_value.cend();
    }

    bool empty() const {
        return begin() == end();
    }

    void clear() {
        m_value.clear();
    }

    inline bool isKeyExist( const Key& key ) const {
        return m_value.end() != m_value.find( key );
    }

    inline IValue* operator[]( const Key& key ) const {
        auto itr = m_value.find( key );
        return m_value.end() == itr ? nullptr : itr->second.get();
    }

    inline const auto& get() const {
        return m_value;
    }

    IntValue* getInt( const Key& ) const;
    StringValue* getString( const Key& ) const;

    using IValue::toJson;
    std::string toJson() const override;

private:
    ValMap m_value;
};

}  // namespace common
