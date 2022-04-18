
#pragma once

#include <IValue.h>

namespace common {

class FloatValue : public IValue {
public:
    using UPtr = std::unique_ptr<FloatValue>;
    using FloatType = float;

    static constexpr FloatType Epsilon = 0.001f;  //  0.1 %

    FloatValue() = default;

    FloatValue( const FloatValue& ) = default;

    FloatValue( float value );

    void accept( IValueVisitorConst& visitor ) const override;

    void accept( IValueVisitor& visitor ) override;

    FloatType get() const {
        return m_value;
    }

    void set( FloatType value ) {
        m_value = value;
    }

private:
    FloatType m_value = 0.0f;
};

}  // namespace common
