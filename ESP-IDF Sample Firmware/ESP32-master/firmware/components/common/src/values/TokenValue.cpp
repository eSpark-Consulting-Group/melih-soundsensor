#include <values/TokenValue.h>
#include <IValueVisitor.h>

namespace common {

void TokenValue::accept( common::IValueVisitorConst& visitor ) const {
    visitor.visit( *this );
}

void TokenValue::accept( common::IValueVisitor& visitor ) {
    visitor.visit( *this );
}

}  // namespace common
