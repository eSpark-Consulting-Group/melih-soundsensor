#include "../include/values/TableValue.h"
#include <IValueVisitor.h>
#include <JsonStreamGenerator.h>
#include <IWriteStream.h>
#include <IValueCast.h>

#include <utility>

namespace common {

void TableValue::accept( IValueVisitorConst& visitor ) const {
    visitor.visit( *this );
}

void TableValue::accept( IValueVisitor& visitor ) {
    visitor.visit( *this );
}

void TableValue::insert( const Key& key, ValPtr&& value ) {
    m_value[key] = std::move( value );
}

TableValue::ValPtr TableValue::remove( const Key& key ) {
    auto itr = m_value.find( key );
    if( m_value.end() == itr ) {
        return nullptr;
    }

    auto value = std::move( itr->second );
    m_value.erase( itr );

    return std::move( value );
}

std::string TableValue::toJson() const {
    ::common::VectorWStream stream{ 1024 };
    ::common::JsonStreamGenerator gen{ stream };
    for( const auto& pair : m_value ) {
        gen.writeProperty( pair.first.c_str(), pair.second.get() );
    }
    gen.writeClosingSymbol();
    return stream.toString();
}

IntValue* TableValue::getInt( const Key& key ) const {
    return ::common::iValueCast<IntValue>( operator[]( key ) );
}

StringValue* TableValue::getString( const Key& key ) const {
    return ::common::iValueCast<StringValue>( operator[]( key ) );
}

}  // end of namespace common
