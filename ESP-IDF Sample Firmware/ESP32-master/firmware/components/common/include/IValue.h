#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <algorithm>
#include <functional>

namespace common {

class BoolValue;
class FloatValue;
class IntValue;
class NotificationValue;
class RgbValue;
class ScalebleValue;
class StringValue;
class UserCodeValue;
class ArrayValue;
class TableValue;

class IValueVisitorConst;
class IValueVisitor;
class IReadStream;
class IWriteStream;
class JsonStreamGenerator;

class IValue {
public:
    enum IValueId : std::size_t {
        FloatVal,
        ScalebleVal,
        IntVal,
        BoolVal,
        StringVal,
        RgbVal,
        NotificationVal,
        ArrayVal,
        EndOfArrayVal,
        TableVal,
        EndOfTableVal,
        TokenVal,
        UserCodeVal,
        Int64Val,
        // Add new item before "NotDefined"
        NotDefined
    };

    using UPtr = std::unique_ptr<IValue>;
    using KeyValueInitList = std::initializer_list<std::pair<std::string, std::unique_ptr<::common::IValue>>>;

    static UPtr create( IReadStream& stream );
    static UPtr create( bool boolValue );
    static UPtr create( float floatValue );
    static UPtr create( int intValue );
    static UPtr create( uint32_t intValue );
    static UPtr create( int64_t largeIntValue );
    static UPtr create( const char* stringValue );
    static UPtr create( std::string stringValue );
    static UPtr create( UPtr value );

    /**
     * Creates a TableValue object out of a given sequence of key-value pairs.
     */
    static UPtr create( KeyValueInitList&& table );

    /**
     * Creates an ArrayValue object out of a given sequence of values of a given type.
     * @tparam ElementType Type of elements in a given sequence. An overload of IValue::create must exist for this type.
     * @param sequence A container to be iterated whose elements will be converted to IValue-s.
     * @return A unique_ptr pointing to an ArrayValue.
     */
    template <typename ElementType>
    static UPtr create( std::vector<ElementType> sequence ) {
        auto begin = sequence.begin(), end = sequence.end();
        return createArray( [&]() { return begin != end ? IValue::create( *begin++ ) : UPtr{}; } );
    }

    /**
     * Creates an ArrayValue object out of a given sequence of values of a given type and transforms each value of the
     * original sequence by calling a @param transform function.
     * @tparam ElementType The type of elements in a given @param sequence.
     * @tparam TransformFunctionType The type of a @param transform function.
     *                               Must accept a single argument of @tparam ElementType and return a value compatible
     *                               with IValue::create.
     * @param sequence A container to be iterated.
     * @param transform A function that converts an element of a @param sequence to a value of type compatible with
     *                  IValue::create.
     * @return A unique_ptr pointing to an ArrayValue.
     */
    template <typename ElementType, typename TransformFunctionType>
    static UPtr create( std::vector<ElementType> sequence, TransformFunctionType&& transform ) {
        auto begin = sequence.begin(), end = sequence.end();
        return createArray( [&]() { return begin != end ? IValue::create( transform( *begin++ ) ) : UPtr{}; } );
    }

    /**
     * Creates an ArrayValue and initializes it with the values returned by @param elemGenerator.
     * @param elemGenerator A function generating elements. Must return nullptr to terminate a sequence.
     * @return A unique_ptr pointing to an ArrayValue.
     */
    static UPtr createArray( std::function<UPtr()> elemGenerator );

    virtual ~IValue() = default;

    virtual void accept( IValueVisitorConst& visitor ) const = 0;

    virtual void accept( IValueVisitor& visitor ) = 0;

    void accept( IValueVisitorConst&& visitor ) const;

    UPtr clone() const;

    std::string name() const;

    IValueId getId() const;

    bool write( IWriteStream& stream ) const;
    std::string toJson( const char* rootName ) const;
    virtual std::string toJson() const;

    size_t calculateSerializedSize() const;

    void logInfo( const int offset = 0 ) const;
};

bool operator>( const IValue& lval, const IValue& rval );

bool operator<( const IValue& lval, const IValue& rval );

bool operator==( const IValue& lval, const IValue& rval );

inline bool operator!=( const IValue& lval, const IValue& rval ) {
    return !( lval == rval );
}

inline bool operator>=( const IValue& lval, const IValue& rval ) {
    return lval == rval || lval > rval;
}

inline bool operator<=( const IValue& lval, const IValue& rval ) {
    return lval == rval || lval < rval;
}

}  // namespace common

namespace std {

// Override std implementation of this operators for unique_ptr because it not apropriate for unique_ptr<IValue> type
// Most commonly implicitly used in std containers like std::set or std::map, etc

inline bool operator<( const common::IValue::UPtr& lval, const common::IValue::UPtr& rval ) {
    return *lval < *rval;
}

inline bool operator==( const common::IValue::UPtr& lval, const common::IValue::UPtr& rval ) {
    return *lval == *rval;
}

}  // namespace std
