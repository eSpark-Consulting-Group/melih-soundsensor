#include "../include/values/ArrayValue.h"
#include <IValueVisitor.h>

#include <algorithm>

namespace common {

void ArrayValue::accept( IValueVisitorConst& visitor ) const {
    visitor.visit( *this );
}

void ArrayValue::accept( IValueVisitor& visitor ) {
    visitor.visit( *this );
}

void ArrayValue::push_back( ValPtr&& value ) {
    m_value.push_back( std::move( value ) );
}

}  // end of namespace common
