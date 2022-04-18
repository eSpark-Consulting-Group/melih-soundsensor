#include <values/ScalebleValue.h>
#include <DeviceInfo.h>
#include <IValueVisitor.h>

namespace common {

ScalebleValue::ScalebleValue( FloatType value, bool relative )
: FloatValue( value )
, m_isRelative( relative ) {}

ScalebleValue::ScalebleValue( FloatType value, std::string&& scale, bool relative )
: FloatValue( value )
, m_scale( std::move( scale ) )
, m_isRelative( relative ) {}

ScalebleValue::ScalebleValue( FloatType value, const std::string& scale, bool relative )
: FloatValue( value )
, m_scale( scale )
, m_isRelative( relative ) {}

void ScalebleValue::accept( common::IValueVisitorConst& visitor ) const {
    visitor.visit( *this );
}

void ScalebleValue::accept( common::IValueVisitor& visitor ) {
    visitor.visit( *this );
}

}  // namespace common
