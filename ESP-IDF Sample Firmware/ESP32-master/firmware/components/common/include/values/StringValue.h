
#pragma once

#include "IValue.h"

#include <string>
#include <cinttypes>

namespace common {

class Buffer;

class StringValue : public IValue {
public:
    using UPtr = std::unique_ptr<StringValue>;
    using StringType = std::string;

    StringValue() = default;

    StringValue( const StringType& value )
    : m_value( value ) {}

    StringValue( StringType&& value )
    : m_value( std::move( value ) ) {}

    StringValue( const char* value, uint16_t size ) {
        if( value == nullptr )
            m_value = std::string();
        else
            m_value = StringType( value, size );
    }

    StringValue( const char* value )
    : m_value( value == nullptr ? std::string() : value ) {}

    void accept( IValueVisitorConst& visitor ) const override;

    void accept( IValueVisitor& visitor ) override;

    const StringType& get() const {
        return m_value;
    }

    void set( const char* value, uint16_t size ) {
        if( value == nullptr )
            m_value = std::string();
        else
            m_value = StringType( value, size );
    }

    inline void set( const StringType&& str ) {
        m_value = std::move( str );
    }

private:
    StringType m_value;
};

}  // namespace common
