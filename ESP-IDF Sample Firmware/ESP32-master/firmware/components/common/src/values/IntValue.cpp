#include <values/IntValue.h>
#include <IValueVisitor.h>

namespace common {
IntValue::IntValue( IntType value )
: m_value( value ) {}

void IntValue::accept( common::IValueVisitorConst& visitor ) const {
    visitor.visit( *this );
}

void IntValue::accept( common::IValueVisitor& visitor ) {
    visitor.visit( *this );
}

}  // namespace common
