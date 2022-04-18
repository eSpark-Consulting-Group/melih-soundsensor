#pragma once

#include "IValue.h"
#include <list>
#include <memory>

namespace common {

class ArrayValue : public IValue {
public:
    using UPtr = std::unique_ptr<ArrayValue>;
    using ValPtr = IValue::UPtr;
    using ValList = std::list<ValPtr>;
    using Itr = ValList::iterator;
    using ConstItr = ValList::const_iterator;

public:
    ArrayValue() = default;

    void accept( IValueVisitorConst& visitor ) const override;

    void accept( IValueVisitor& visitor ) override;

    void push_back( ValPtr&& value );

    Itr begin() {
        return m_value.begin();
    }

    Itr end() {
        return m_value.end();
    }

    ConstItr begin() const {
        return m_value.cbegin();
    }

    ConstItr end() const {
        return m_value.cend();
    }

    ConstItr cbegin() const {
        return m_value.cbegin();
    }

    ConstItr cend() const {
        return m_value.cend();
    }

    inline const auto& get() const {
        return m_value;
    }

private:
    ValList m_value;
};

}  // namespace common
