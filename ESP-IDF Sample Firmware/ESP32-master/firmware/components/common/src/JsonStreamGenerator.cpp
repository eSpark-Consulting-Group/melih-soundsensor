
#include <JsonStreamGenerator.h>
#include <cmath>
#include <IWriteStream.h>
#include <IReadStream.h>

#include <IValueVisitor.h>
#include <CommonResources.h>
#include <values/IntValue.h>
#include <values/Int64Value.hpp>
#include <values/FloatValue.h>
#include <values/ScalebleValue.h>
#include <values/BoolValue.h>
#include <values/StringValue.h>
#include <values/RgbValue.h>
#include <values/NotificationValue.h>
#include <values/ArrayValue.h>
#include <values/TableValue.h>
#include <values/UserCodeValue.h>
#include <values/TokenValue.h>

#include <gason.hpp>

#include <string.h>
#include <cstdio>
#include <limits>

#include <DebugUtils.h>

namespace common {
namespace {

constexpr char TrueValue[] = "true";
constexpr char FalseValue[] = "false";
constexpr char NullValue[] = "null";

constexpr auto ObjectBracketStart = '{';
constexpr auto ObjectBracketEnd = '}';

constexpr auto ArrayBracketStart = '[';
constexpr auto ArrayBracketEnd = ']';

constexpr auto StringValueQuote = '\"';
constexpr auto PropertyDelimiter = ':';
constexpr auto CommaDelimiter = ',';

constexpr auto MinimalNumberLength = 64;

class IValueJsonificator final : public common::IValueVisitorConst {
public:
    IValueJsonificator( common::JsonStreamGenerator& jsonGenerator, const char* fieldId )
    : m_jsonGenerator( jsonGenerator )
    , m_fieldId( fieldId ) {}

    void visit( const common::FloatValue& value ) override {
        m_result = m_result && m_jsonGenerator.writeProperty( m_fieldId, value.get() );
    }

    void visit( const common::ScalebleValue& value ) override {
        m_result = m_jsonGenerator.writeProperty( m_fieldId, value.get() );

        if( m_fieldId != std::string( common::resources::tokens::MinValue ) &&
            m_fieldId != std::string( common::resources::tokens::MaxValue ) ) {
            m_result =
                m_result && m_jsonGenerator.writeProperty( common::resources::tokens::Scale, value.getScale().c_str() );
        }
    }

    void visit( const common::UserCodeValue& value ) override {
        m_result = m_jsonGenerator.writePropertyKey( m_fieldId );
        m_result = m_result && m_jsonGenerator.writeOpeningObject();

        m_result = m_result && m_jsonGenerator.writeProperty( common::resources::tokens::Code, value.getCode() );
        m_result = m_result && m_jsonGenerator.writeProperty( common::resources::tokens::Name, value.getName() );

        m_result = m_result && m_jsonGenerator.writeClosingSymbol();
    }

    void visit( const common::IntValue& value ) override {
        m_result = m_jsonGenerator.writeProperty( m_fieldId, value.get() );
    }

    void visit( const common::Int64Value& value ) override {
        m_result = m_jsonGenerator.writeProperty( m_fieldId, value.get() );
    }

    void visit( const common::BoolValue& value ) override {
        m_result = m_jsonGenerator.writeProperty( m_fieldId, value.get() );
    }

    void visit( const common::StringValue& value ) override {
        m_result = m_jsonGenerator.writeProperty( m_fieldId, value.get() );
    }

    void visit( const common::TokenValue& value ) override {
        m_result = m_jsonGenerator.writeProperty( m_fieldId, value.get() );
    }

    void visit( const common::RgbValue& value ) override {
        m_result = m_jsonGenerator.writePropertyKey( m_fieldId );
        m_result = m_result && m_jsonGenerator.writeOpeningObject();
        for( const common::RgbValue::ColorComponent& component : value.get() ) {
            auto key = common::RgbValue::componentIdToStr( component.color );
            if( !key ) {
                continue;
            }
            m_result = m_result && m_jsonGenerator.writeProperty( key, int( component.value ) );
        }
        m_result = m_result && m_jsonGenerator.writeClosingSymbol();
    }

    void visit( const common::ArrayValue& value ) override {
        m_result = m_jsonGenerator.writePropertyKey( m_fieldId );
        m_result = m_result && m_jsonGenerator.writeOpeningArray();
        IValueJsonificator visitor( m_jsonGenerator, nullptr );
        if( m_result ) {
            for( const auto& val : value ) {
                val->accept( visitor );
            }
        }
        m_result = m_result && m_jsonGenerator.writeClosingSymbol();
    }

    void visit( const common::TableValue& value ) override {
        m_result = m_jsonGenerator.writePropertyKey( m_fieldId );
        m_result = m_result && m_jsonGenerator.writeOpeningObject();
        if( m_result ) {
            for( const auto& val : value ) {
                IValueJsonificator visitor( m_jsonGenerator, val.first.c_str() );
                val.second->accept( visitor );
            }
        }
        m_result = m_result && m_jsonGenerator.writeClosingSymbol();
    }

    void visit( const common::NotificationValue& value ) override {
        m_result = false;
    }

    bool isOk() const {
        return m_result;
    }

private:
    static std::string timeStr( uint8_t hour, uint8_t min, uint8_t sec ) {
        const char timeFormat[] = "%02d:%02d:%02d";
        std::array<char, sizeof( timeFormat )> buffer;
        ::snprintf( buffer.data(), buffer.size(), timeFormat, hour, min, sec );
        return buffer.data();
    }

    static std::string dateTimeStr( uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec ) {
        const char dateTimeFormat[] = "%04d-%02d-%02dT%02d:%02d:%02d";
        std::array<char, sizeof( dateTimeFormat )> buffer;
        ::snprintf( buffer.data(), buffer.size(), dateTimeFormat, year, month, day, hour, min, sec );
        return buffer.data();
    }

private:
    common::JsonStreamGenerator& m_jsonGenerator;
    const char* m_fieldId;
    bool m_result = true;
};

}  // namespace

JsonStreamGenerator::JsonStreamGenerator( IWriteStream& outputStream )
: m_outputStream( outputStream ) {}

JsonStreamGenerator::~JsonStreamGenerator() {
    writeJsonEnd();
}

bool JsonStreamGenerator::writeJsonEnd() {
    while( m_isOk && m_depth > 0 ) {
        m_isOk = writeClosingSymbol();
    }

    return m_isOk;
}

bool JsonStreamGenerator::writePropertyDelimiter() {
    m_isOk = m_isOk && write( PropertyDelimiter );
    return m_isOk;
}

bool JsonStreamGenerator::writeItemDelimiter() {
    m_isOk = m_isOk && write( CommaDelimiter );
    return m_isOk;
}

bool JsonStreamGenerator::writeNullValue() {
    m_isOk = m_isOk && write( NullValue, sizeof( NullValue ) - 1 );
    return m_isOk;
}

bool JsonStreamGenerator::writeEmptyObject() {
    m_isOk = m_isOk && write( ObjectBracketStart );
    m_isOk = m_isOk && write( ObjectBracketEnd );
    return m_isOk;
}

bool JsonStreamGenerator::writeEmptyArray() {
    return writeOpeningArray() && writeClosingSymbol();
}

bool JsonStreamGenerator::writePropertyKey( const char* key, std::uint16_t keySize ) {
    m_isOk = checkAndWriteOpeningBrackets();
    m_isOk = m_isOk && checkAndWriteComma();

    if( key ) {
        m_isOk = m_isOk && writeStringProperty( key, keySize );
        m_isOk = m_isOk && write( PropertyDelimiter );
    }

    m_state = State::ValuePart;
    return m_isOk;
}

bool JsonStreamGenerator::writePropertyKey( const char* key ) {
    return writePropertyKey( key, key ? strlen( key ) : 0 );
}

bool JsonStreamGenerator::writePropertyValue( const char* value, std::uint16_t valueSize ) {
    if( value ) {
        m_isOk = m_isOk && writeStringProperty( value, valueSize );
    }
    else {
        m_isOk = m_isOk && write( NullValue, sizeof( NullValue ) - 1 );
    }

    m_state = State::Item;
    return m_isOk;
}

bool JsonStreamGenerator::writePropertyValue( const char* value ) {
    return writePropertyValue( value, value ? strlen( value ) : 0 );
}

bool JsonStreamGenerator::writeProperty( const char* key,
                                         std::uint16_t keySize,
                                         const char* value,
                                         std::uint16_t valueSize ) {
    m_isOk = m_isOk && writePropertyKey( key, keySize );

    return writePropertyValue( value, valueSize );
}

bool JsonStreamGenerator::writeProperty( const char* key, const char* value, std::uint16_t valueSize ) {
    return writeProperty( key, key ? strlen( key ) : 0, value, valueSize );
}

bool JsonStreamGenerator::writeProperty( const char* key, const char* value ) {
    return writeProperty( key, value, value ? strlen( value ) : 0 );
}

bool JsonStreamGenerator::writeProperty( const char* key, std::uint16_t keySize, float value ) {
    if( std::isnan( value ) ) {
        return writeProperty( key, nullptr );
    }
    else if( std::isinf( value ) ) {
        return writeProperty( key, std::numeric_limits<float>::max() );
    }

    std::array<char, MinimalNumberLength> bufferForNumber;

    m_isOk = writePropertyKey( key, keySize );

    m_isOk = m_isOk && ( snprintf( bufferForNumber.data(), bufferForNumber.size(), "%.5f", value ) > 0 );
    m_isOk = m_isOk && write( bufferForNumber.data() );

    m_state = State::Item;

    return m_isOk;
}

bool JsonStreamGenerator::writeProperty( const char* key, std::uint16_t keySize, std::uint16_t value ) {
    std::array<char, MinimalNumberLength> bufferForNumber;

    m_isOk = writePropertyKey( key, keySize );

    m_isOk = m_isOk && ( snprintf( bufferForNumber.data(), bufferForNumber.size(), "%d", value ) > 0 );
    m_isOk = m_isOk && write( bufferForNumber.data() );

    m_state = State::Item;

    return m_isOk;
}

bool JsonStreamGenerator::writeProperty( const char* key, std::uint16_t keySize, std::uint32_t value ) {
    std::array<char, MinimalNumberLength> bufferForNumber;

    m_isOk = writePropertyKey( key, keySize );

    m_isOk = m_isOk && ( snprintf( bufferForNumber.data(), bufferForNumber.size(), "%" PRIu32, value ) > 0 );
    m_isOk = m_isOk && write( bufferForNumber.data() );

    m_state = State::Item;

    return m_isOk;
}

bool JsonStreamGenerator::writeProperty( const char* key, std::uint16_t keySize, std::int16_t value ) {
    std::array<char, MinimalNumberLength> bufferForNumber;

    m_isOk = writePropertyKey( key, keySize );

    m_isOk = m_isOk && ( snprintf( bufferForNumber.data(), bufferForNumber.size(), "%" PRId16, value ) > 0 );
    m_isOk = m_isOk && write( bufferForNumber.data() );

    m_state = State::Item;

    return m_isOk;
}

bool JsonStreamGenerator::writeProperty( const char* key, std::uint16_t keySize, std::int32_t value ) {
    std::array<char, MinimalNumberLength> bufferForNumber;

    m_isOk = writePropertyKey( key, keySize );

    m_isOk = m_isOk && ( snprintf( bufferForNumber.data(), bufferForNumber.size(), "%" PRId32, value ) > 0 );
    m_isOk = m_isOk && write( bufferForNumber.data() );

    m_state = State::Item;

    return m_isOk;
}

bool JsonStreamGenerator::writeProperty( const char* key, float value ) {
    return writeProperty( key, key ? strlen( key ) : 0, std::isnan( value ) ? float( 0.0 ) : value );
}

bool JsonStreamGenerator::writeProperty( const char* key, std::uint16_t keySize, std::uint64_t value ) {
    std::array<char, MinimalNumberLength> bufferForNumber;

    m_isOk = writePropertyKey( key, keySize );

    m_isOk = m_isOk && ( snprintf( bufferForNumber.data(), bufferForNumber.size(), "%" PRIu64, value ) > 0 );
    m_isOk = m_isOk && write( bufferForNumber.data() );

    m_state = State::Item;

    return m_isOk;
}

bool JsonStreamGenerator::writeProperty( const char* key, std::uint16_t keySize, std::int64_t value ) {
    std::array<char, MinimalNumberLength> bufferForNumber;

    m_isOk = writePropertyKey( key, keySize );

    m_isOk = m_isOk && ( snprintf( bufferForNumber.data(), bufferForNumber.size(), "%" PRId64, value ) > 0 );
    m_isOk = m_isOk && write( bufferForNumber.data() );

    m_state = State::Item;

    return m_isOk;
}

bool JsonStreamGenerator::writeProperty( const char* key, std::uint16_t value ) {
    return writeProperty( key, key ? strlen( key ) : 0, value );
}

bool JsonStreamGenerator::writeProperty( const char* key, std::uint32_t value ) {
    return writeProperty( key, key ? strlen( key ) : 0, value );
}

bool JsonStreamGenerator::writeProperty( const char* key, std::int16_t value ) {
    return writeProperty( key, key ? strlen( key ) : 0, value );
}

bool JsonStreamGenerator::writeProperty( const char* key, std::int32_t value ) {
    return writeProperty( key, key ? strlen( key ) : 0, value );
}

bool JsonStreamGenerator::writeProperty( const char* key, std::uint64_t value ) {
    return writeProperty( key, key ? strlen( key ) : 0, value );
}

bool JsonStreamGenerator::writeProperty( const char* key, std::int64_t value ) {
    return writeProperty( key, key ? strlen( key ) : 0, value );
}

bool JsonStreamGenerator::writeProperty( const char* key, std::uint16_t keySize, bool value ) {
    m_isOk = m_isOk && writePropertyKey( key, keySize );
    m_isOk = m_isOk &&
             ( value ? write( TrueValue, sizeof( TrueValue ) - 1 ) : write( FalseValue, sizeof( FalseValue ) - 1 ) );

    m_state = State::Item;

    return m_isOk;
}

bool JsonStreamGenerator::writeProperty( const char* key, bool value ) {
    return writeProperty( key, key ? strlen( key ) : 0, value );
}

bool JsonStreamGenerator::writeProperty( const char* key, const IValue& value ) {
    IValueJsonificator visitor( *this, key );
    value.accept( visitor );
    return visitor.isOk();
}

bool JsonStreamGenerator::writeProperty( const char* key, const IValue* value ) {
    if( !value ) {
        return writePropertyKey( key ) && writeNullValue() && writeItemDelimiter();
    }

    return writeProperty( key, *value );
}

bool JsonStreamGenerator::writeProperty( const char* key, const gason::JsonValue& value ) {
    switch( value.getTag() ) {
    case gason::JsonTag::JSON_NUMBER: {
        bool isSuccess;
        auto numVal = value.toInt( &isSuccess );
        if( !isSuccess ) {
            return false;
        }
        return writeProperty( key, numVal );
    }
    case gason::JsonTag::JSON_STRING: {
        auto valStr = value.toString();
        if( !valStr ) {
            return false;
        }
        return writeProperty( key, valStr );
    }
    case gason::JsonTag::JSON_ARRAY: {
        if( !writePropertyKey( key ) || !writeOpeningArray() ) {
            return false;
        }
        auto itrEnd = gason::end( value );
        for( auto itr = gason::begin( value ); itr != itrEnd; ++itr ) {
            if( !writeProperty( nullptr, itr->value ) ) {
                return false;
            }
        }
        return writeClosingSymbol();
    }
    case gason::JsonTag::JSON_OBJECT: {
        if( !writePropertyKey( key ) || !writeOpeningObject() ) {
            return false;
        }
        auto itrEnd = gason::end( value );
        for( auto itr = gason::begin( value ); itr != itrEnd; ++itr ) {
            if( !writeProperty( itr->key, itr->value ) ) {
                return false;
            }
        }
        return writeClosingSymbol();
    }
    case gason::JsonTag::JSON_TRUE:
        return writeProperty( key, true );
    case gason::JsonTag::JSON_FALSE:
        return writeProperty( key, false );
    case gason::JsonTag::JSON_NULL:
        return writeProperty( key, nullptr );
    default:
        return false;
    }
}

bool JsonStreamGenerator::writeRawProperty( const char* key, const char* value ) {
    m_isOk = m_isOk && writePropertyKey( key );

    if( value ) {
        m_isOk = m_isOk && write( value );
    }
    else {
        m_isOk = m_isOk && write( NullValue, sizeof( NullValue ) - 1 );
    }

    m_state = State::Item;
    return m_isOk;
}

bool JsonStreamGenerator::writeRawProperty( const char* key, IReadStream& stream ) {
    m_isOk = m_isOk && writePropertyKey( key );

    if( 0 < stream.size() ) {
        if( m_isOk ) {
            char rbuf[128];
            while( true ) {
                size_t readed = stream.read( rbuf, sizeof( rbuf ) );
                m_isOk = m_outputStream.write( rbuf, readed ) == readed;
                if( sizeof( rbuf ) > readed || !m_isOk )
                    break;
            }
        }
    }
    else {
        m_isOk = m_isOk && write( NullValue, sizeof( NullValue ) - 1 );
    }

    m_state = State::Item;
    return m_isOk;
}

bool JsonStreamGenerator::writeRawProperty( const char* key, const storage::KeyValueStorage::BlobType& blob ) {
    m_isOk = m_isOk && writePropertyKey( key ) && write( &blob.front(), blob.size() );
    return m_isOk;
}

bool JsonStreamGenerator::write( const char symbol ) {
    return write( &symbol, sizeof( symbol ) );
}

bool JsonStreamGenerator::write( const char* property, const int size ) {
    return m_outputStream.write( property, size ) == size;
}

bool JsonStreamGenerator::write( const char* property ) {
    return write( property, property ? strlen( property ) : 0 );
}

bool JsonStreamGenerator::writeStringProperty( const char* property, std::uint16_t size ) {
    m_isOk = write( StringValueQuote );
    for( const char* ch = property; m_isOk && ch != property + size; ++ch ) {
        switch( *ch ) {
        case '\\':
            m_isOk = write( '\\' ) && write( '\\' );
            break;
        case '"':
            m_isOk = write( '\\' ) && write( '"' );
            break;
        case '\b':
            m_isOk = write( '\\' ) && write( 'b' );
            break;
        case '\f':
            m_isOk = write( '\\' ) && write( 'f' );
            break;
        case '\n':
            m_isOk = write( '\\' ) && write( 'n' );
            break;
        case '\r':
            m_isOk = write( '\\' ) && write( 'r' );
            break;
        case '\t':
            m_isOk = write( '\\' ) && write( 't' );
            break;
        default:
            m_isOk = write( *ch );
        }
    }
    m_isOk = m_isOk && write( StringValueQuote );
    return m_isOk;
}

bool JsonStreamGenerator::checkAndWriteComma() {
    if( m_state == State::Item ) {
        m_isOk = m_isOk && write( CommaDelimiter );
    }

    return m_isOk;
}

bool JsonStreamGenerator::checkAndWriteOpeningBrackets() {
    if( m_depth == 0 ) {
        if( writeOpeningObject() ) {
            m_state = State::FirstItem;
        }
        else {
            m_isOk = false;
        }
    }

    return m_isOk;
}

bool JsonStreamGenerator::writeOpeningObject() {
    if( ( m_depth < MaxDepthSize ) && checkAndWriteComma() && write( ObjectBracketStart ) ) {
        m_bracketsStack |= ( 1 << m_depth++ );
        m_state = State::FirstItem;
        return true;
    }

    return false;
}

bool JsonStreamGenerator::writeOpeningArray() {
    if( ( m_depth < MaxDepthSize ) && write( ArrayBracketStart ) ) {
        if( m_bracketsStack & ( 1 << m_depth ) ) {
            m_bracketsStack ^= ( 1 << m_depth );
        }

        ++m_depth;
        m_state = State::FirstItem;
        return true;
    }

    return false;
}

bool JsonStreamGenerator::writeClosingSymbol() {
    if( ( m_depth > 0 ) &&
        write( ( ( m_bracketsStack & ( 1 << ( m_depth - 1 ) ) ) ? ObjectBracketEnd : ArrayBracketEnd ) ) ) {
        --m_depth;
        m_state = m_depth == 0 ? State::Root : State::Item;
        return true;
    }

    return false;
}

}  // namespace common
