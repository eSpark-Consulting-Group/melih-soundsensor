#include "IValue.h"
#include "IReadStream.h"
#include "IWriteStream.h"
#include "IValueVisitor.h"
#include "IValueCast.h"

#include <values/FloatValue.h>
#include <values/ScalebleValue.h>
#include <values/IntValue.h>
#include <values/BoolValue.h>
#include <values/StringValue.h>
#include <values/RgbValue.h>
#include <values/NotificationValue.h>
#include <values/ArrayValue.h>
#include <values/TableValue.h>
#include <values/UserCodeValue.h>
#include <values/TokenValue.h>
#include <values/Int64Value.hpp>
#include "converters/ScalesConverterVisitor.hpp"

#include "JsonStreamGenerator.h"
#include "CommonResources.h"

#include <cmath>
#include <iostream>
#include <iomanip>

//#define ENABLE_IVL_LOGS
#ifdef ENABLE_IVL_LOGS

#include <DebugUtils.h>

#define IVL_LOG_TAG "[IValue.cpp] "
#define IVL_LOGI( _FMT, ... ) ESP_LOGI( IVL_LOG_TAG, _FMT, ##__VA_ARGS__ )
#define IVL_LOGE( _FMT, ... ) ESP_LOGE( IVL_LOG_TAG, _FMT, ##__VA_ARGS__ )
#define IVL_LOGW( _FMT, ... ) ESP_LOGW( IVL_LOG_TAG, _FMT, ##__VA_ARGS__ )

#else

#define IVL_LOGI( ... ) \
    do {                \
    } while( false )
#define IVL_LOGE( ... ) \
    do {                \
    } while( false )
#define IVL_LOGW( ... ) \
    do {                \
    } while( false )

#endif

namespace common {

namespace {

/**
 * Changes introduced into this serializer MUST comply with IValueSerializedSizeCalculator.
 */
class SerializeVisitor : public IValueVisitorConst {
    IWriteStream& m_stream;
    bool m_isSuccess = true;

public:
    SerializeVisitor( IWriteStream& stream )
    : m_stream( stream ){};

    operator bool() const {
        return m_isSuccess;
    }

    bool serialize( const IValue& value ) {
        value.accept( *this );
        return *this;
    }

    void visit( const FloatValue& value ) override {
        IVL_LOGI( "serialize FloatValue" );
        m_isSuccess = m_isSuccess && m_stream.write( std::size_t( IValue::FloatVal ) ) && m_stream.write( value.get() );
    }

    void visit( const ScalebleValue& value ) override {
        IVL_LOGI( "serialize ScalebleValue" );
        std::uint8_t type = 0;  // TODO for backward compatibility
        m_isSuccess = m_isSuccess && m_stream.write( std::size_t( IValue::ScalebleVal ) ) &&
                      m_stream.write( value.get() ) && m_stream.write( type );  // TODO for backward compatibility
        std::string scale = value.getScale();
        std::size_t scaleLen = scale.length();
        m_isSuccess =
            m_isSuccess && m_stream.write( scaleLen ) && ( scaleLen == m_stream.write( scale.c_str(), scaleLen ) );
    }

    void visit( const UserCodeValue& value ) override {
        IVL_LOGI( "serialize UserCodeValue" );
        m_isSuccess = m_isSuccess && m_stream.write( std::size_t( IValue::UserCodeVal ) );
        m_isSuccess = m_isSuccess && m_stream.write( value.getName() );
        m_isSuccess = m_isSuccess && m_stream.write( value.getCode() );
    }

    void visit( const IntValue& value ) override {
        IVL_LOGI( "serialize IntValue" );
        m_isSuccess = m_isSuccess && m_stream.write( std::size_t( IValue::IntVal ) ) && m_stream.write( value.get() );
    }

    void visit( const Int64Value& value ) override {
        IVL_LOGI( "serialize Int64Value" );
        m_isSuccess = m_isSuccess && m_stream.write( std::size_t( IValue::Int64Val ) ) && m_stream.write( value.get() );
    }

    void visit( const BoolValue& value ) override {
        IVL_LOGI( "serialize BoolValue" );
        m_isSuccess = m_isSuccess && m_stream.write( std::size_t( IValue::BoolVal ) ) && m_stream.write( value.get() );
    }

    void visit( const StringValue& value ) override {
        IVL_LOGI( "serialize StringValue" );
        m_isSuccess = m_isSuccess && m_stream.write( std::size_t( IValue::StringVal ) );
        m_isSuccess = m_isSuccess && m_stream.write( value.get() );
    }

    void visit( const TokenValue& value ) override {
        IVL_LOGI( "serialize TokenValue" );
        m_isSuccess = m_isSuccess && m_stream.write( std::size_t( IValue::TokenVal ) );
        m_isSuccess = m_isSuccess && m_stream.write( value.get() );
    }

    void visit( const RgbValue& value ) override {
        IVL_LOGI( "serialize RgbValue" );
        m_isSuccess = m_isSuccess && m_stream.write( std::size_t( IValue::RgbVal ) );
        const RgbValue::RgbData& data = value.get();
        std::size_t itemsCount = data.size();
        m_isSuccess = m_isSuccess && m_stream.write( itemsCount );
        for( int i = 0; i < itemsCount && m_isSuccess; ++i ) {
            const std::uint8_t color = std::uint8_t( data[i].color );
            m_isSuccess = m_isSuccess && m_stream.write( color );
            const std::uint8_t value = data[i].value;
            m_isSuccess = m_isSuccess && m_stream.write( value );
        }
    }

    void visit( const NotificationValue& value ) override {
        IVL_LOGI( "serialize NotificationValue" );
        m_isSuccess = m_isSuccess && m_stream.write( std::size_t( IValue::NotificationVal ) ) &&
                      m_stream.write( std::uint8_t( value.getType() ) ) &&
                      m_stream.write( std::uint8_t( value.getEvent() ) ) &&
                      m_stream.write( std::uint8_t( value.getAlarmType() ) ) &&
                      m_stream.write( std::uint8_t( value.getAlarmLevel() ) );
        std::size_t itemsCount = value.getData().size();
        std::size_t dataSize = itemsCount * sizeof( NotificationValue::BlobValueType );
        m_isSuccess =
            m_isSuccess && m_stream.write( itemsCount ) &&
            ( dataSize == m_stream.write( reinterpret_cast<const char*>( value.getData().data() ), dataSize ) );
    };

    void visit( const ArrayValue& value ) override {
        IVL_LOGI( "serialize ArrayValue" );
        m_stream.write( std::size_t( IValue::ArrayVal ) );
        for( auto& val : value ) {
            val->accept( *this );
        }
        m_stream.write( std::size_t( IValue::EndOfArrayVal ) );
    }

    void visit( const TableValue& value ) override {
        IVL_LOGI( "serialize TableValue" );
        m_stream.write( std::size_t( IValue::TableVal ) );
        auto itr = value.cbegin();
        auto end = value.cend();
        for( ; itr != end; ++itr ) {
            auto key = itr->first;
            StringValue keyVal( key.c_str(), key.length() );
            keyVal.accept( *this );
            itr->second->accept( *this );
        }
        m_stream.write( std::size_t( IValue::EndOfTableVal ) );
    }
};

}  // namespace

void IValue::accept( IValueVisitorConst&& visitor ) const {
    accept( visitor );
}

IValue::UPtr IValue::create( IReadStream& stream ) {
    std::size_t id;
    if( !stream.read( id ) ) {
        return nullptr;
    }

    switch( id ) {
    case FloatVal: {
        IVL_LOGI( "deserialize FloatValue" );
        FloatValue::FloatType value;
        return stream.read( value ) ? std::make_unique<FloatValue>( value ) : nullptr;
    }
    case ScalebleVal: {
        IVL_LOGI( "deserialize ScalebleValue" );
        ScalebleValue::FloatType value;
        std::uint8_t type;  // TODO for backward compatibility
        std::string scale;
        if( !stream.read( value ) || !stream.read( type ) || !stream.read( scale ) ) {
            return nullptr;
        }
        return std::make_unique<ScalebleValue>( value, std::move( scale ) );
    }
    case IntVal: {
        IVL_LOGI( "deserialize IntValue" );
        IntValue::IntType value;
        return stream.read( value ) ? std::make_unique<IntValue>( value ) : nullptr;
    }
    case Int64Val: {
        IVL_LOGI( "deserialize Int64Value" );
        int64_t value;
        return stream.read( value ) ? std::make_unique<Int64Value>( value ) : nullptr;
    }
    case BoolVal: {
        IVL_LOGI( "deserialize BoolValue" );
        bool value;
        return stream.read( value ) ? std::make_unique<BoolValue>( value ) : nullptr;
    }
    case StringVal: {
        IVL_LOGI( "deserialize StringValue" );
        StringValue::StringType str;
        return stream.read( str ) ? std::make_unique<StringValue>( std::move( str ) ) : nullptr;
    }
    case TokenVal: {
        IVL_LOGI( "deserialize TokenValue" );
        std::string str;
        return stream.read( str ) ? std::make_unique<TokenValue>( std::move( str ) ) : nullptr;
    }
    case RgbVal: {
        IVL_LOGI( "deserialize RgbValue" );
        std::size_t itemsCount;
        if( !stream.read( itemsCount ) ) {
            return nullptr;
        }
        RgbValue::RgbData data;
        data.reserve( itemsCount );
        for( int i = 0; i < itemsCount; ++i ) {
            std::uint8_t color, value;
            if( !stream.read( color ) || !stream.read( value ) ) {
                return nullptr;
            }
            data.emplace_back( RgbValue::Color( color ), value );
        }
        return std::make_unique<RgbValue>( std::move( data ) );
    }
    case NotificationVal: {
        IVL_LOGI( "deserialize NotificationValue" );

        std::uint8_t notification, event, alarmType, alarmLevel;
        NotificationValue::BlobType data;
        if( !stream.read( notification ) || !stream.read( event ) || !stream.read( alarmType ) ||
            !stream.read( alarmLevel ) || !stream.read( data ) ) {
            return nullptr;
        }
        return std::make_unique<NotificationValue>( alarmType,
                                                    alarmLevel,
                                                    znet::NotificationType( notification ),
                                                    znet::NotificationEvent( event ),
                                                    std::move( data ) );
    }
    case ArrayVal: {
        IVL_LOGI( "deserialize ArrayValue" );
        auto arrayValue = std::make_unique<ArrayValue>();
        while( true ) {
            auto val = create( stream );
            if( !val ) {
                break;
            }
            arrayValue->push_back( std::move( val ) );
        }
        return arrayValue;
    }

    case TableVal: {
        IVL_LOGI( "deserialize TableValue" );
        auto tableValue = std::make_unique<TableValue>();
        while( true ) {
            auto key = iValueCast<StringValue>( create( stream ) );
            if( !key ) {
                return tableValue;
            }
            auto val = create( stream );
            if( !val ) {
                return nullptr;
            }

            tableValue->insert( key->get(), std::move( val ) );
        }
        return tableValue;
    }
    case UserCodeVal: {
        IVL_LOGI( "deserialize UserCodeValue" );
        std::string name;
        std::string code;
        if( !stream.read( name ) || !stream.read( code ) ) {
            return nullptr;
        }
        return std::make_unique<UserCodeValue>( std::move( name ), std::move( code ) );
    }
    case EndOfArrayVal:
    case EndOfTableVal:
    default:
        return nullptr;
    }
}

IValue::UPtr IValue::create( bool boolValue ) {
    return std::make_unique<::common::BoolValue>( boolValue );
}

IValue::UPtr IValue::create( float floatValue ) {
    return std::make_unique<::common::FloatValue>( floatValue );
}

IValue::UPtr IValue::create( int intValue ) {
    return std::make_unique<::common::IntValue>( intValue );
}

IValue::UPtr IValue::create( uint32_t intValue ) {
    return std::make_unique<::common::IntValue>( intValue );
}

IValue::UPtr IValue::create( int64_t largeIntValue ) {
    return std::make_unique<::common::Int64Value>( largeIntValue );
}

IValue::UPtr IValue::create( const char* stringValue ) {
    return std::make_unique<::common::StringValue>( stringValue );
}

IValue::UPtr IValue::create( std::string stringValue ) {
    return std::make_unique<::common::StringValue>( std::move( stringValue ) );
}

IValue::UPtr IValue::create( UPtr value ) {
    return value;
}

IValue::UPtr IValue::create( KeyValueInitList&& table ) {
    auto value = std::make_unique<::common::TableValue>();
    for( auto&& element : table ) {
        value->insert( element.first, element.second ? element.second->clone() : std::unique_ptr<::common::IValue>{} );
    }
    return value;
}

IValue::UPtr IValue::createArray( std::function<UPtr()> elemGenerator ) {
    auto array = std::make_unique<::common::ArrayValue>();
    while( auto elem = elemGenerator() ) {
        array->push_back( std::move( elem ) );
    }
    return array;
}

IValue::UPtr IValue::clone() const {
    using namespace common::resources;

    struct CloneVisitor : public IValueVisitorConst {
        IValue::UPtr m_value;

        void visit( const FloatValue& value ) override {
            m_value = std::make_unique<FloatValue>( value );
        }

        void visit( const ScalebleValue& value ) override {
            m_value = std::make_unique<ScalebleValue>( value );
        }

        void visit( const IntValue& value ) override {
            m_value = std::make_unique<IntValue>( value );
        }

        void visit( const Int64Value& value ) override {
            m_value = std::make_unique<Int64Value>( value );
        }

        void visit( const BoolValue& value ) override {
            m_value = std::make_unique<BoolValue>( value );
        }

        void visit( const StringValue& value ) override {
            m_value = std::make_unique<StringValue>( value );
        }

        void visit( const TokenValue& value ) override {
            m_value = std::make_unique<TokenValue>( value );
        }

        void visit( const RgbValue& value ) override {
            m_value = std::make_unique<RgbValue>( value );
        }

        void visit( const NotificationValue& value ) override {
            m_value = std::make_unique<NotificationValue>( value );
        }

        void visit( const ArrayValue& value ) override {
            auto cloned_val = std::make_unique<ArrayValue>();
            if( !cloned_val ) {
                return;
            }

            for( auto& child_val : value.get() ) {
                auto cloned_child_val = child_val->clone();
                if( !cloned_child_val ) {
                    return;
                }
                cloned_val->push_back( std::move( cloned_child_val ) );
            };
            m_value = std::move( cloned_val );
        }

        void visit( const TableValue& value ) override {
            auto cloned_val = std::make_unique<TableValue>();
            if( !cloned_val ) {
                return;
            }

            for( auto& child_val : value.get() ) {
                auto cloned_child_val = child_val.second->clone();
                if( !cloned_child_val ) {
                    return;
                }
                cloned_val->insert( child_val.first, std::move( cloned_child_val ) );
            };

            m_value = std::move( cloned_val );
        }

        void visit( const UserCodeValue& value ) override {
            m_value = std::make_unique<UserCodeValue>( value );
        }

        IValue::UPtr clone( const IValue& ival ) {
            ival.accept( *this );
            return std::move( m_value );
        }
    };

    CloneVisitor visitor;
    return visitor.clone( *this );
}

std::string IValue::name() const {
    using namespace common::resources;

    struct NameVisitor : public IValueVisitorConst {
        std::string& m_name;

        NameVisitor( std::string& name )
        : m_name( name ){};

        void visit( const FloatValue& value ) override {
            m_name = values::Float;
        }

        void visit( const ScalebleValue& value ) override {
            m_name = values::Scaleble;
        }

        void visit( const IntValue& value ) override {
            m_name = values::Int;
        }

        void visit( const Int64Value& value ) override {
            m_name = values::Int64;
        }

        void visit( const BoolValue& value ) override {
            m_name = values::Bool;
        }

        void visit( const StringValue& value ) override {
            m_name = values::String;
        }

        void visit( const TokenValue& value ) override {
            m_name = values::Token;
        }

        void visit( const RgbValue& value ) override {
            m_name = values::RGB;
        }

        void visit( const NotificationValue& value ) override {
            m_name = values::RGB;
        }

        void visit( const ArrayValue& value ) override {
            m_name = values::Array;
        }

        void visit( const TableValue& value ) override {
            m_name = values::Table;
        }

        void visit( const UserCodeValue& value ) override {
            m_name = values::Usercode;
        }
    };

    std::string name;
    NameVisitor visitor( name );
    this->accept( visitor );
    return std::move( name );
}

IValue::IValueId IValue::getId() const {
    struct IdVisitor : public IValueVisitorConst {
        IValueId m_id;

        void visit( const FloatValue& value ) override {
            m_id = IValueId::FloatVal;
        }

        void visit( const ScalebleValue& value ) override {
            m_id = IValueId::ScalebleVal;
        }

        void visit( const IntValue& value ) override {
            m_id = IValueId::IntVal;
        }

        void visit( const Int64Value& value ) override {
            m_id = IValueId::Int64Val;
        }

        void visit( const BoolValue& value ) override {
            m_id = IValueId::BoolVal;
        }

        void visit( const StringValue& value ) override {
            m_id = IValueId::StringVal;
        }

        void visit( const TokenValue& value ) override {
            m_id = IValueId::TokenVal;
        }

        void visit( const RgbValue& value ) override {
            m_id = IValueId::RgbVal;
        }

        void visit( const NotificationValue& value ) override {
            m_id = IValueId::NotificationVal;
        }

        void visit( const ArrayValue& value ) override {
            m_id = IValueId::ArrayVal;
        }

        void visit( const TableValue& value ) override {
            m_id = IValueId::TableVal;
        }

        void visit( const UserCodeValue& value ) override {
            m_id = IValueId::UserCodeVal;
        }

        inline IValueId get( const IValue& ival ) {
            m_id = IValueId::NotDefined;
            ival.accept( *this );
            return m_id;
        }
    };

    return IdVisitor().get( *this );
}

std::string IValue::toJson( const char* rootName ) const {
    ::common::VectorWStream stream{ 1024 };
    ::common::JsonStreamGenerator gen{ stream };
    if( gen.writeProperty( rootName, *this ) ) {
        gen.writeClosingSymbol();
        return stream.toString();
    }
    return {};
}

std::string IValue::toJson() const {
    return toJson( nullptr );
}

bool IValue::write( IWriteStream& stream ) const {
    common::SerializeVisitor serializator( stream );
    return serializator.serialize( *this );
}
void IValue::logInfo( const int offset ) const {
    std::cout << "IValue: type=" << name();

    struct Visitor : public IValueVisitorConst {
        const int m_offset;
        std::ostream& m_stream;

        Visitor( const int offset, std::ostream& stream )
        : m_offset( offset )
        , m_stream( stream ) {
            stream << std::setw( offset );
        }

        void visit( const FloatValue& rval ) override {
            m_stream << ", value=" << rval.get();
        }

        void visit( const ScalebleValue& rval ) override {
            m_stream << ", value=" << rval.get() << ", scale=" << rval.getScale();
        }

        void visit( const IntValue& rval ) override {
            m_stream << ", value=" << rval.get();
        }

        void visit( const Int64Value& rval ) override {
            m_stream << ", value=" << rval.get();
        }

        void visit( const BoolValue& rval ) override {
            m_stream << ", value=" << rval.get();
        }

        void visit( const StringValue& rval ) override {
            m_stream << ", value=" << rval.get();
        }

        void visit( const TokenValue& rval ) override {
            m_stream << ", value=" << rval.get();
        }

        void visit( const RgbValue& rval ) override {}

        void visit( const NotificationValue& rval ) override {}

        void visit( const ArrayValue& rval ) override {
            m_stream << ", [\n";
            const auto end = rval.cend();
            for( auto itr = rval.cbegin(); end != itr; ++itr ) {
                ( *itr )->logInfo( m_offset + 1 );
                m_stream << ",\n";
            }
            m_stream << "]\n";
        }

        void visit( const TableValue& rval ) override {
            m_stream << ", {\n";
            const auto end = rval.cend();
            for( auto itr = rval.cbegin(); end != itr; ++itr ) {
                m_stream << itr->first << ": ";
                itr->second->logInfo( m_offset + 1 );
                m_stream << ",\n";
            }
            m_stream << "}\n";
        }

        void visit( const UserCodeValue& rval ) override {}

        inline void print( const IValue& lval ) {
            lval.accept( *this );
        }
    };

    Visitor infoPrinter( offset, std::cout );
    infoPrinter.print( *this );

    if( 0 == offset ) {
        std::cout << std::endl;
    }
}

bool operator>( const IValue& lval, const IValue& rval ) {
    struct Visitor : public IValueVisitorConst {
        const IValue& m_lval;
        bool m_isMore = false;

        Visitor( const IValue& lval )
        : m_lval( lval ) {}

        void visit( const FloatValue& rval ) override {
            auto lval = iValueCast<FloatValue>( &m_lval );
            if( lval ) {
                m_isMore = lval->get() > rval.get();
            }
        }

        void visit( const ScalebleValue& rval ) override {
            auto lval = iValueCast<ScalebleValue>( &m_lval );
            if( lval ) {
                if( !lval->getScale().empty() && !rval.getScale().empty() ) {
                    ScalebleValue tempval = rval;
                    ScalesConverterBaseVisitor visitor( lval->getScale() );
                    tempval.accept( visitor );
                    m_isMore = lval->get() > tempval.get();
                }
            }
        }

        void visit( const IntValue& rval ) override {
            auto lval = iValueCast<IntValue>( &m_lval );
            if( lval ) {
                m_isMore = lval->get() > rval.get();
            }
        }

        void visit( const Int64Value& rval ) override {
            auto lval = iValueCast<Int64Value>( &m_lval );
            if( lval ) {
                m_isMore = lval->get() > rval.get();
            }
        }

        void visit( const BoolValue& rval ) override {
            auto lval = iValueCast<BoolValue>( &m_lval );
            if( lval ) {
                m_isMore = lval->get() > rval.get();
            }
        }

        void visit( const StringValue& rval ) override {
            auto lval = iValueCast<StringValue>( &m_lval );
            if( lval ) {
                m_isMore = lval->get() > rval.get();
            }
        }

        void visit( const TokenValue& rval ) override {
            auto lval = iValueCast<TokenValue>( &m_lval );
            if( lval ) {
                m_isMore = lval->get() > rval.get();
            }
        }

        void visit( const RgbValue& rval ) override {
            auto lval = iValueCast<RgbValue>( &m_lval );
            if( lval ) {
                m_isMore = lval->get() > rval.get();
            }
        }

        void visit( const NotificationValue& rval ) override {
            auto lval = iValueCast<NotificationValue>( &m_lval );
            if( lval ) {
                m_isMore = lval->getAlarmType() == rval.getAlarmType() &&
                           lval->getAlarmLevel() > rval.getAlarmLevel() && lval->getType() == rval.getType() &&
                           lval->getEvent() == rval.getEvent();  // && lval->getData() > rval.getData(); //
            }
        }

        void visit( const ArrayValue& rval ) override {
            auto lval = iValueCast<ArrayValue>( &m_lval );
            if( lval ) {
                m_isMore = lval->get() > rval.get();
            }
        }

        void visit( const TableValue& rval ) override {
            auto lval = iValueCast<TableValue>( &m_lval );
            if( lval ) {
                m_isMore = lval->get() > rval.get();
            }
        }

        void visit( const UserCodeValue& rval ) override {
            auto lval = iValueCast<UserCodeValue>( &m_lval );
            if( lval ) {
                m_isMore = lval->getName() == rval.getName() && lval->getCode() > rval.getCode();
            }
        }

        inline bool compare( const IValue& lval ) {
            lval.accept( *this );
            return m_isMore;
        }
    };

    return Visitor( lval ).compare( rval );
}

bool operator<( const IValue& lval, const IValue& rval ) {
    struct Visitor : public IValueVisitorConst {
        const IValue& m_lval;
        bool m_isLess = false;

        Visitor( const IValue& lval )
        : m_lval( lval ) {}

        void visit( const FloatValue& rval ) override {
            auto lval = iValueCast<FloatValue>( &m_lval );
            if( lval ) {
                m_isLess = lval->get() < rval.get();
            }
        }

        void visit( const ScalebleValue& rval ) override {
            auto lval = iValueCast<ScalebleValue>( &m_lval );
            if( lval ) {
                if( !lval->getScale().empty() && !rval.getScale().empty() ) {
                    ScalebleValue tempval = rval;
                    ScalesConverterBaseVisitor visitor( lval->getScale() );

                    tempval.accept( visitor );
                    m_isLess = lval->get() < tempval.get();
                }
            }
        }

        void visit( const IntValue& rval ) override {
            auto lval = iValueCast<IntValue>( &m_lval );
            if( lval ) {
                m_isLess = lval->get() < rval.get();
            }
        }

        void visit( const Int64Value& rval ) override {
            auto lval = iValueCast<Int64Value>( &m_lval );
            if( lval ) {
                m_isLess = lval->get() < rval.get();
            }
        }

        void visit( const BoolValue& rval ) override {
            auto lval = iValueCast<BoolValue>( &m_lval );
            if( lval ) {
                m_isLess = lval->get() < rval.get();
            }
        }

        void visit( const StringValue& rval ) override {
            auto lval = iValueCast<StringValue>( &m_lval );
            if( lval ) {
                m_isLess = lval->get() < rval.get();
            }
        }

        void visit( const TokenValue& rval ) override {
            auto lval = iValueCast<TokenValue>( &m_lval );
            if( lval ) {
                m_isLess = lval->get() < rval.get();
            }
        }

        void visit( const RgbValue& rval ) override {
            auto lval = iValueCast<RgbValue>( &m_lval );
            if( lval ) {
                m_isLess = lval->get() < rval.get();
            }
        }

        void visit( const NotificationValue& rval ) override {
            auto lval = iValueCast<NotificationValue>( &m_lval );
            if( lval ) {
                m_isLess = lval->getAlarmType() == rval.getAlarmType() &&
                           lval->getAlarmLevel() < rval.getAlarmLevel() && lval->getType() == rval.getType() &&
                           lval->getEvent() == rval.getEvent();  // && lval->getData() < rval.getData(); //
            }
        }

        void visit( const ArrayValue& rval ) override {
            auto lval = iValueCast<ArrayValue>( &m_lval );
            if( lval ) {
                m_isLess = lval->get() < rval.get();
            }
        }

        void visit( const TableValue& rval ) override {
            auto lval = iValueCast<TableValue>( &m_lval );
            if( lval ) {
                m_isLess = lval->get() < rval.get();
            }
        }

        void visit( const UserCodeValue& rval ) override {
            auto lval = iValueCast<UserCodeValue>( &m_lval );
            if( lval ) {
                m_isLess = lval->getName() == rval.getName() && lval->getCode() < rval.getCode();
            }
        }

        inline bool compare( const IValue& lval ) {
            lval.accept( *this );
            return m_isLess;
        }
    };

    return Visitor( lval ).compare( rval );
}

bool operator==( const IValue& lval, const IValue& rval ) {
    struct Visitor : public IValueVisitorConst {
        const IValue& m_lval;
        bool m_isEqual = false;

        Visitor( const IValue& lval )
        : m_lval( lval ) {}

        void visit( const FloatValue& rval ) override {
            auto lval = iValueCast<FloatValue>( &m_lval );
            if( lval ) {
                m_isEqual = std::fabs( lval->get() - rval.get() ) <=
                            ( FloatValue::Epsilon * ( std::fabs( lval->get() ) < std::fabs( rval.get() ) ?
                                                          std::fabs( rval.get() ) :
                                                          std::fabs( lval->get() ) ) );
            }
        }

        void visit( const ScalebleValue& rval ) override {
            auto lval = iValueCast<ScalebleValue>( &m_lval );
            if( lval ) {
                m_isEqual = lval->get() == rval.get() && lval->getScale() == rval.getScale();
                if( !m_isEqual ) {
                    ScalebleValue tempval = rval;

                    ScalesConverterBaseVisitor visitor( lval->getScale() );
                    tempval.accept( visitor );
                    m_isEqual = lval->getScale() == tempval.getScale() &&
                                std::fabs( lval->get() - tempval.get() ) <=
                                    ( FloatValue::Epsilon * ( std::fabs( lval->get() ) < std::fabs( tempval.get() ) ?
                                                                  std::fabs( tempval.get() ) :
                                                                  std::fabs( lval->get() ) ) );
                }
            }
        }

        void visit( const IntValue& rval ) override {
            auto lval = iValueCast<IntValue>( &m_lval );
            if( lval ) {
                m_isEqual = lval->get() == rval.get();
            }
        }

        void visit( const Int64Value& rval ) override {
            auto lval = iValueCast<Int64Value>( &m_lval );
            if( lval ) {
                m_isEqual = lval->get() == rval.get();
            }
        }

        void visit( const BoolValue& rval ) override {
            auto lval = iValueCast<BoolValue>( &m_lval );
            if( lval ) {
                m_isEqual = lval->get() == rval.get();
            }
        }

        void visit( const StringValue& rval ) override {
            auto lval = iValueCast<StringValue>( &m_lval );
            if( lval ) {
                m_isEqual = lval->get() == rval.get();
            }
        }

        void visit( const TokenValue& rval ) override {
            auto lval = iValueCast<TokenValue>( &m_lval );
            if( lval ) {
                m_isEqual = lval->get() == rval.get();
            }
        }

        void visit( const RgbValue& rval ) override {
            auto lval = iValueCast<RgbValue>( &m_lval );
            if( lval ) {
                m_isEqual = lval->get() == rval.get();
            }
        }

        void visit( const NotificationValue& rval ) override {
            auto lval = iValueCast<NotificationValue>( &m_lval );
            if( lval ) {
                m_isEqual = lval->getAlarmType() == rval.getAlarmType() &&
                            lval->getAlarmLevel() == rval.getAlarmLevel() && lval->getType() == rval.getType() &&
                            lval->getEvent() == rval.getEvent() && lval->getData() == rval.getData();
            }
        }

        void visit( const ArrayValue& rval ) override {
            auto lval = iValueCast<ArrayValue>( &m_lval );
            if( lval ) {
                m_isEqual = lval->get() == rval.get();
            }
        }

        void visit( const TableValue& rval ) override {
            auto lval = iValueCast<TableValue>( &m_lval );
            if( lval ) {
                m_isEqual = lval->get() == rval.get();
            }
        }

        void visit( const UserCodeValue& rval ) override {
            auto lval = iValueCast<UserCodeValue>( &m_lval );
            if( lval ) {
                m_isEqual = lval->getName() == rval.getName() && lval->getCode() == rval.getCode();
            }
        }

        inline bool compare( const IValue& lval ) {
            lval.accept( *this );
            return m_isEqual;
        }
    };

    return Visitor( lval ).compare( rval );
}

class IValueSerializedSizeCalculator : IValueVisitorConst {
public:
    void visit( const FloatValue& value ) override {
        m_totalSize += sizeof( size_t ) + sizeof( value.get() );
    }
    void visit( const ScalebleValue& value ) override {
        m_totalSize += /* IValue::ScalebleVal */ sizeof( size_t ) + sizeof( value.get() ) +
                       /* backward compatibility byte */ sizeof( std::uint8_t ) + /* scale length */ sizeof( size_t ) +
                       /* scale string */ value.getScale().length();
    }
    void visit( const IntValue& value ) override {
        m_totalSize += sizeof( size_t ) + sizeof( value.get() );
    }
    void visit( const Int64Value& value ) override {
        m_totalSize += sizeof( size_t ) + sizeof( value.get() );
    }
    void visit( const BoolValue& value ) override {
        m_totalSize += sizeof( size_t ) + sizeof( value.get() );
    }
    void visit( const StringValue& value ) override {
        m_totalSize += /* IValue::StringVal */ sizeof( size_t ) + /* string length */ sizeof( size_t ) +
                       /* string itself */ value.get().length();
    }
    void visit( const TokenValue& value ) override {
        m_totalSize += /* IValue::TokenVal */ sizeof( size_t ) + /* token string length */ sizeof( size_t ) +
                       /* token string */ value.get().length();
    }
    void visit( const RgbValue& value ) override {
        m_totalSize += /* IValue::RgbVal */ sizeof( size_t ) + /* number of colors */ sizeof( size_t ) +
                       value.get().size() * ( /* color */ sizeof( std::uint8_t ) + /* value */ sizeof( std::uint8_t ) );
    }
    void visit( const NotificationValue& value ) override {
        m_totalSize += /* IValue::NotificationVal */ sizeof( size_t ) + /* type */ sizeof( std::uint8_t ) +
                       /* event */ sizeof( std::uint8_t ) + /* alarmType */ sizeof( std::uint8_t ) +
                       /* alarmLevel */ sizeof( std::uint8_t ) +
                       /* data size */ sizeof( size_t ) +
                       /* data itself */ value.getData().size() * sizeof( NotificationValue::BlobValueType );
    }
    void visit( const UserCodeValue& value ) override {
        m_totalSize += /* IValue::UserCodeVal */ sizeof( size_t ) + /* name string length */ sizeof( size_t ) +
                       /* name string */ value.getName().length() + /* code string length */ sizeof( size_t ) +
                       /* code string */ value.getCode().length();
    }
    void visit( const ArrayValue& value ) override {
        m_totalSize += /* IValue::ArrayVal */ sizeof( size_t ) + /* IValue::EndOfArrayVal */ sizeof( size_t );
        for( auto& arrayElem : value ) {
            arrayElem->accept( *this );
        }
    }
    void visit( const TableValue& value ) override {
        m_totalSize += /* IValue::TableVal */ sizeof( size_t ) + /* IValue::EndOfTableVal */ sizeof( size_t );
        for( auto& tableElem : value ) {
            m_totalSize += /* IValue::StringVal */ sizeof( size_t ) + /* key string length */ sizeof( size_t ) +
                           /* key string */ tableElem.first.length();
            tableElem.second->accept( *this );
        }
    }

    size_t calculate( const ::common::IValue& value ) {
        value.accept( *this );
        return m_totalSize;
    }

private:
    size_t m_totalSize = 0;
};

size_t IValue::calculateSerializedSize() const {
    return IValueSerializedSizeCalculator{}.calculate( *this );
}

}  // namespace common
