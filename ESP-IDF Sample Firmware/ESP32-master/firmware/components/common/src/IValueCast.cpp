#include "IValueCast.h"

#include "values/IntValue.h"
#include "values/Int64Value.hpp"
#include "values/BoolValue.h"

#include "DeviceInfo.h"

#include "DebugUtils.h"

#include <cmath>
#include <cstdlib>

namespace common {

namespace {

class IValueCastVisitorConstBase : public IValueVisitorConst {
public:
    void visit( const FloatValue& value ) override {}

    void visit( const ScalebleValue& value ) override {}

    void visit( const IntValue& value ) override {}

    void visit( const Int64Value& value ) override {}

    void visit( const BoolValue& value ) override {}

    void visit( const StringValue& value ) override {}

    void visit( const TokenValue& value ) override {}

    void visit( const RgbValue& value ) override {}

    void visit( const NotificationValue& value ) override {}

    void visit( const UserCodeValue& value ) override {}

    void visit( const ArrayValue& value ) override {}

    void visit( const TableValue& value ) override {}
};

}  // namespace

template <>
std::unique_ptr<ScalebleValue> iValueCast<ScalebleValue>( IValue::UPtr&& value ) {
    class Visitor : public IValueCastVisitorBase {
        IValue::UPtr& m_valueToConvert;
        std::unique_ptr<ScalebleValue> m_value;

    public:
        Visitor( IValue::UPtr& valueToConvert )
        : m_valueToConvert( valueToConvert ) {}

        void visit( ScalebleValue& value ) override {
            m_value.reset( &value );
            if( m_value ) {
                m_valueToConvert.release();
            }
        };

        void visit( TableValue& value ) override {
            auto scale = iValueCast<StringValue>( value[common::resources::tokens::Scale] );
            if( !scale ) {
                return;
            }

            auto valueIValPtr = value[common::resources::tokens::Value];
            if( !valueIValPtr ) {
                return;
            }
            auto val = iValueCast<float>( *valueIValPtr );
            if( std::isnan( val ) ) {
                return;
            }

            auto relative = iValueCast<BoolValue>( value[common::resources::tokens::Relative] );
            if( relative && relative->get() ) {
                m_value = std::make_unique<ScalebleValue>( val, scale->get(), relative->get() );
            }
            else {
                m_value = std::make_unique<ScalebleValue>( val, scale->get() );
            }
        };

        std::unique_ptr<ScalebleValue> convert() {
            if( !m_valueToConvert ) {
                return nullptr;
            }
            m_valueToConvert->accept( *this );
            return std::move( m_value );
        }
    };

    return Visitor( value ).convert();
}

template <>
std::unique_ptr<UserCodeValue> iValueCast<UserCodeValue>( IValue::UPtr&& value ) {
    class Visitor : public IValueCastVisitorBase {
        IValue::UPtr& m_valueToConvert;
        std::unique_ptr<UserCodeValue> m_value;

    public:
        Visitor( IValue::UPtr& valueToConvert )
        : m_valueToConvert( valueToConvert ) {}

        void visit( UserCodeValue& value ) override {
            m_value.reset( &value );
            if( m_value ) {
                m_valueToConvert.release();
            }
        };

        void visit( TableValue& value ) override {
            auto code = iValueCast<StringValue>( value[common::resources::tokens::Code] );
            if( !code ) {
                return;
            }

            auto name = iValueCast<StringValue>( value[common::resources::tokens::Name] );
            if( !name ) {
                return;
            }

            m_value = std::make_unique<UserCodeValue>( name->get(), code->get() );
        };

        std::unique_ptr<UserCodeValue> convert() {
            if( !m_valueToConvert ) {
                return nullptr;
            }
            m_valueToConvert->accept( *this );
            return std::move( m_value );
        }
    };

    return Visitor( value ).convert();
}

template <>
std::unique_ptr<RgbValue> iValueCast<RgbValue>( IValue::UPtr&& value ) {
    class Visitor : public IValueCastVisitorBase {
        IValue::UPtr& m_valueToConvert;
        std::unique_ptr<RgbValue> m_value;

    public:
        Visitor( IValue::UPtr& valueToConvert )
        : m_valueToConvert( valueToConvert ) {}

        void visit( RgbValue& value ) override {
            m_value.reset( &value );
            if( m_value ) {
                m_valueToConvert.release();
            }
        };

        void visit( TableValue& value ) override {
            if( value.isKeyExist( common::resources::tokens::Value ) ) {
                m_value = iValueCast<RgbValue>( value.remove( common::resources::tokens::Value ) );
                return;
            }

            RgbValue::RgbData data;
            data.reserve( 9 );

            const auto end = RgbValue::Color::Indexed;
            for( auto idx = RgbValue::Color::Wwhite; idx <= end; ++idx ) {
                auto intVal = iValueCast<IntValue>( std::move( value.remove( RgbValue::componentIdToStr( idx ) ) ) );
                if( intVal ) {
                    data.emplace_back( RgbValue::Color( idx ), intVal.get()->get() );
                }
            }

            if( !data.empty() ) {
                m_value = std::make_unique<RgbValue>( std::move( data ) );
            }
        };

        std::unique_ptr<RgbValue> convert() {
            if( !m_valueToConvert ) {
                return nullptr;
            }
            m_valueToConvert->accept( *this );
            return std::move( m_value );
        }
    };

    return Visitor( value ).convert();
}

template <>
std::unique_ptr<IntValue> iValueCast<IntValue>( IValue::UPtr&& value ) {
    class Visitor : public IValueCastVisitorBase {
        IValue::UPtr& m_valueToConvert;
        std::unique_ptr<IntValue> m_value;

    public:
        Visitor( IValue::UPtr& valueToConvert )
        : m_valueToConvert( valueToConvert ) {}

        void visit( IntValue& value ) override {
            m_value.reset( &value );
            if( m_value ) {
                m_valueToConvert.release();
            }
        };

        void visit( FloatValue& value ) override {
            m_value = std::make_unique<IntValue>( value.get() );
        };

        void visit( StringValue& value ) override {
            m_value = std::make_unique<IntValue>( std::atoi( value.get().c_str() ) );
        };

        void visit( TableValue& value ) override {
            m_value = iValueCast<IntValue>( value.remove( common::resources::tokens::Value ) );
        };

        std::unique_ptr<IntValue> convert() {
            if( !m_valueToConvert ) {
                return nullptr;
            }
            m_valueToConvert->accept( *this );
            return std::move( m_value );
        }
    };
    return Visitor( value ).convert();
}

template <>
float iValueCast<float>( const IValue& value ) {
    class FloatVisitor : public IValueCastVisitorConstBase {
        float m_value = NAN;

    public:
        void visit( const FloatValue& value ) override {
            m_value = value.get();
        };

        void visit( const ScalebleValue& value ) override {
            m_value = value.get();
        };

        void visit( const IntValue& value ) override {
            m_value = value.get();
        };

        void visit( const Int64Value& value ) override {
            m_value = value.get();
        };

        void visit( const StringValue& value ) override {
            m_value = std::atof( value.get().c_str() );
        };

        void visit( const BoolValue& value ) override {
            m_value = value.get() ? 1.0 : 0.0;
        }

        void visit( const TableValue& value ) override {
            auto ivalPtr = value[common::resources::tokens::Value];
            if( ivalPtr ) {
                m_value = iValueCast<float>( *ivalPtr );
            }
        }

        float value( const IValue& value ) {
            value.accept( *this );
            return m_value;
        }
    };

    return FloatVisitor().value( value );
}

template <>
std::unique_ptr<FloatValue> iValueCast<FloatValue>( IValue::UPtr&& value ) {
    class Visitor : public IValueCastVisitorBase {
        IValue::UPtr& m_valueToConvert;
        std::unique_ptr<FloatValue> m_value;

    public:
        Visitor( IValue::UPtr& valueToConvert )
        : m_valueToConvert( valueToConvert ) {}

        void visit( FloatValue& value ) override {
            m_value.reset( &value );
            if( m_value ) {
                m_valueToConvert.release();
            }
        };

        std::unique_ptr<FloatValue> convert() {
            if( !m_valueToConvert ) {
                return nullptr;
            }
            m_valueToConvert->accept( *this );
            return std::move( m_value );
        }
    };

    auto convertedValue = Visitor( value ).convert();
    if( convertedValue ) {
        return std::move( convertedValue );
    }

    auto fVal = iValueCast<float>( value );
    return std::isnan( fVal ) ? nullptr : std::make_unique<FloatValue>( fVal );
}

template <>
std::unique_ptr<StringValue> iValueCast<StringValue>( IValue::UPtr&& value ) {
    class Visitor : public IValueCastVisitorBase {
        IValue::UPtr& m_valueToConvert;
        std::unique_ptr<StringValue> m_value;

    public:
        Visitor( IValue::UPtr& valueToCast )
        : m_valueToConvert( valueToCast ) {}

        void visit( StringValue& value ) override {
            m_value.reset( &value );
            if( m_value ) {
                m_valueToConvert.release();
            }
        };

        void visit( TokenValue& value ) override {
            m_value = std::make_unique<StringValue>( value.get() );
        };

        void visit( TableValue& value ) override {
            auto type = iValueCast<StringValue>( value.remove( common::resources::tokens::Type ) );
            if( type ) {
                if( common::resources::tokens::Token != type->get() &&
                    common::resources::values::String != type->get() ) {
                    return;
                }
            }
            m_value = iValueCast<StringValue>( value.remove( common::resources::tokens::Value ) );
        };

        std::unique_ptr<StringValue> convert() {
            if( !m_valueToConvert ) {
                return nullptr;
            }
            m_valueToConvert->accept( *this );
            return std::move( m_value );
        }
    };

    return Visitor( value ).convert();
}

template <>
StringValue* iValueCast<StringValue>( IValue* value ) {
    class Visitor : public IValueCastVisitorBase {
        StringValue* m_value;

    public:
        void visit( TokenValue& value ) override {
            m_value = &value;
        };

        void visit( StringValue& value ) override {
            m_value = &value;
        };

        void visit( TableValue& value ) override {
            auto type = iValueCast<StringValue>( value[common::resources::tokens::Type] );
            if( type ) {
                if( common::resources::tokens::Token != type->get() &&
                    common::resources::values::String != type->get() ) {
                    return;
                }
            }
            m_value = iValueCast<StringValue>( value[common::resources::tokens::Value] );
        };

        StringValue* convert( IValue* value ) {
            value->accept( *this );
            return m_value;
        }
    };

    return value ? Visitor().convert( value ) : nullptr;
}

template <>
std::unique_ptr<TokenValue> iValueCast<TokenValue>( IValue::UPtr&& value ) {
    class Visitor : public IValueCastVisitorBase {
        IValue::UPtr& m_valueToConvert;
        std::unique_ptr<TokenValue> m_value;

    public:
        Visitor( IValue::UPtr& valueToCast )
        : m_valueToConvert( valueToCast ) {}

        void visit( TokenValue& value ) override {
            m_value.reset( &value );
            if( m_value ) {
                m_valueToConvert.release();
            }
        };

        void visit( StringValue& value ) override {
            m_value = std::make_unique<TokenValue>( value.get() );
        };

        void visit( TableValue& value ) override {
            m_value = iValueCast<TokenValue>( value.remove( common::resources::tokens::Value ) );
        };

        std::unique_ptr<TokenValue> convert() {
            if( !m_valueToConvert ) {
                return nullptr;
            }
            m_valueToConvert->accept( *this );
            return std::move( m_value );
        }
    };

    return Visitor( value ).convert();
}

template <>
std::unique_ptr<BoolValue> iValueCast<BoolValue>( IValue::UPtr&& value ) {
    class Visitor : public IValueCastVisitorBase {
        IValue::UPtr& m_valueToConvert;
        std::unique_ptr<BoolValue> m_value;

    public:
        Visitor( IValue::UPtr& valueToCast )
        : m_valueToConvert( valueToCast ) {}

        void visit( BoolValue& value ) override {
            m_value.reset( &value );
            if( m_value ) {
                m_valueToConvert.release();
            }
        };

        void visit( StringValue& value ) override {
            if( value.get() == resources::tokens::True ) {
                m_value = std::make_unique<BoolValue>( true );
            }
            else if( value.get() == resources::tokens::False ) {
                m_value = std::make_unique<BoolValue>( false );
            }
        };

        void visit( IntValue& value ) override {
            m_value = std::make_unique<BoolValue>( 0 != value.get() );
        };

        void visit( FloatValue& value ) override {
            m_value = std::make_unique<BoolValue>( 0.0 != value.get() );
        };

        void visit( TableValue& value ) override {
            m_value = iValueCast<BoolValue>( value.remove( common::resources::tokens::Value ) );
        };

        std::unique_ptr<BoolValue> convert() {
            if( !m_valueToConvert ) {
                return nullptr;
            }
            m_valueToConvert->accept( *this );
            return std::move( m_value );
        }
    };

    return Visitor( value ).convert();
}

template <>
std::unique_ptr<TableValue> iValueCast<TableValue>( IValue::UPtr&& value ) {
    if( !value ) {
        return nullptr;
    }

    struct Visitor : IValueCastVisitorConstBase {
        const TableValue* res = nullptr;

        void visit( const TableValue& value ) override {
            res = &value;
        };

    } visitor;

    value->accept( visitor );

    return visitor.res ? std::unique_ptr<TableValue>( static_cast<TableValue*>( value.release() ) ) : nullptr;
}

template <>
TableValue* iValueCast<TableValue>( IValue* value ) {
    if( !value ) {
        return nullptr;
    }

    struct Visitor : IValueCastVisitorBase {
        TableValue* res = nullptr;

        void visit( TableValue& value ) override {
            res = &value;
        };
    } visitor;

    value->accept( visitor );

    return visitor.res;
}

}  // end of namespace common
