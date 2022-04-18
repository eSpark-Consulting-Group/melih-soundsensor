#include <values/FloatValue.h>
#include <IValueVisitor.h>

namespace common {

FloatValue::FloatValue( float value )
: m_value( value ) {}

void FloatValue::accept( common::IValueVisitorConst& visitor ) const {
    visitor.visit( *this );
}

void FloatValue::accept( common::IValueVisitor& visitor ) {
    visitor.visit( *this );
}

}  // namespace common
