
#pragma once

#include <IValue.h>
#include <cstdint>

namespace common {

class IntValue final : public IValue {
public:
    using UPtr = std::unique_ptr<IntValue>;

    using IntType = int32_t;

    IntValue() = default;

    IntValue( const IntValue& ) = default;

    IntValue( IntType value );

    void accept( IValueVisitorConst& visitor ) const override;

    void accept( IValueVisitor& visitor ) override;

    IntType get() const {
        return m_value;
    }

    void set( IntType value ) {
        m_value = value;
    }

private:
    IntType m_value = 0;
};

}  // namespace common
