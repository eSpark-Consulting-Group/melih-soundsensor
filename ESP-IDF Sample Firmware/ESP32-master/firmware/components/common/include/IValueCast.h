#pragma once

#include "IValue.h"
#include "IValueVisitor.h"

#include "values/ScalebleValue.h"
#include "values/UserCodeValue.h"
#include "values/RgbValue.h"
#include "values/IntValue.h"
#include "values/FloatValue.h"
#include "values/StringValue.h"
#include "values/TokenValue.h"
#include "values/TableValue.h"
#include "values/BoolValue.h"

#include "CommonResources.h"

#include <memory>
#include <cmath>

namespace common {

class IValueCastVisitorBase : public IValueVisitor {
public:
    void visit( FloatValue& value ) override{};

    void visit( ScalebleValue& value ) override{};

    void visit( IntValue& value ) override{};

    void visit( Int64Value& value ) override{};

    void visit( BoolValue& value ) override{};

    void visit( StringValue& value ) override{};

    void visit( TokenValue& value ) override{};

    void visit( RgbValue& value ) override{};

    void visit( NotificationValue& value ) override{};

    void visit( UserCodeValue& value ) override{};

    void visit( ArrayValue& value ) override{};

    void visit( TableValue& value ) override{};
};

template <typename T>
T iValueCast( const IValue& value );

template <typename T>
T iValueCast( const IValue::UPtr& value );

template <typename T>
std::unique_ptr<T> iValueCast( IValue::UPtr&& valueToCast ) {
    if( !valueToCast ) {
        return nullptr;
    }

    struct Visitor : IValueCastVisitorBase {
        IValue::UPtr& m_valueToCast;
        std::unique_ptr<T> res = nullptr;

        Visitor( IValue::UPtr& valueToCast )
        : m_valueToCast( valueToCast ) {}

        inline void visit( T& value ) override {
            res.reset( &value );
            if( res ) {
                m_valueToCast.release();
            }
        };

        inline void visit( TableValue& value ) override {
            res = iValueCast<T>( value.remove( common::resources::tokens::Value ) );
        };

        inline std::unique_ptr<T> convert() {
            m_valueToCast->accept( *this );
            return std::move( res );
        }
    } visitor( valueToCast );

    return visitor.convert();
}

template <typename T>
T* iValueCast( IValue* value ) {
    if( !value ) {
        return nullptr;
    }

    struct Visitor : IValueCastVisitorBase {
        T* res = nullptr;

        inline void visit( T& value ) override {
            res = &value;
        };

        inline void visit( TableValue& value ) override {
            res = iValueCast<T>( value[common::resources::tokens::Value] );
        };
    } visitor;

    value->accept( visitor );

    return visitor.res;
}

template <typename T>
inline const T* iValueCast( const IValue* value ) {
    return iValueCast<T>( const_cast<IValue*>( value ) );
}

// Conversion to POD types

template <>
float iValueCast<float>( const IValue& value );

template <>
inline float iValueCast<float>( const IValue::UPtr& value ) {
    return value ? iValueCast<float>( *value ) : NAN;
}

// Conversion from common IValue to child types

template <>
std::unique_ptr<ScalebleValue> iValueCast<ScalebleValue>( IValue::UPtr&& value );

template <>
std::unique_ptr<UserCodeValue> iValueCast<UserCodeValue>( IValue::UPtr&& value );

template <>
std::unique_ptr<RgbValue> iValueCast<RgbValue>( IValue::UPtr&& value );

template <>
std::unique_ptr<IntValue> iValueCast<IntValue>( IValue::UPtr&& value );

template <>
std::unique_ptr<FloatValue> iValueCast<FloatValue>( IValue::UPtr&& value );

template <>
std::unique_ptr<StringValue> iValueCast<StringValue>( IValue::UPtr&& value );

template <>
StringValue* iValueCast<StringValue>( IValue* value );

template <>
std::unique_ptr<TokenValue> iValueCast<TokenValue>( IValue::UPtr&& value );

template <>
std::unique_ptr<BoolValue> iValueCast<BoolValue>( IValue::UPtr&& value );

template <>
std::unique_ptr<TableValue> iValueCast<TableValue>( IValue::UPtr&& value );

template <>
TableValue* iValueCast<TableValue>( IValue* value );

}  // end of namespace common
