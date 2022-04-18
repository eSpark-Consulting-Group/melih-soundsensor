#include <values/StringValue.h>
#include <IValueVisitor.h>

namespace common {

void StringValue::accept( common::IValueVisitorConst& visitor ) const {
    visitor.visit( *this );
}

void StringValue::accept( common::IValueVisitor& visitor ) {
    visitor.visit( *this );
}

}  // namespace common
