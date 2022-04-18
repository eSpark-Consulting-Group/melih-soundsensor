#pragma once

#include "IValue.h"
#include "IValueVisitor.h"

namespace common {

class Int64Value final : public IValue {
public:
    Int64Value() = default;

    explicit Int64Value( int64_t value )
    : m_value( value ) {}

    void accept( common::IValueVisitorConst& visitor ) const {
        visitor.visit( *this );
    }

    void accept( common::IValueVisitor& visitor ) {
        visitor.visit( *this );
    }

    auto get() const {
        return m_value;
    }

    void set( int64_t newValue ) {
        m_value = newValue;
    }

private:
    int64_t m_value = 0;
};

}  // namespace common
