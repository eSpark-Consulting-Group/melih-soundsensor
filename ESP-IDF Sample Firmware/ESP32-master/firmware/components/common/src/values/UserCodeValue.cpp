#include <values/UserCodeValue.h>
#include <IValueVisitor.h>

namespace common {

UserCodeValue::UserCodeValue( std::string&& name, std::string&& code )
: m_name( std::move( name ) )
, m_code( std::move( code ) ) {}

UserCodeValue::UserCodeValue( const std::string& name, const std::string& code )
: m_name( name )
, m_code( code ) {}

void UserCodeValue::accept( common::IValueVisitorConst& visitor ) const {
    visitor.visit( *this );
}

void UserCodeValue::accept( common::IValueVisitor& visitor ) {
    visitor.visit( *this );
}

}  // namespace common
