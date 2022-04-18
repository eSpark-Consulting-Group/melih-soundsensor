#pragma once

namespace common {

class IValue;
class FloatValue;
class ScalebleValue;
class IntValue;
class Int64Value;
class BoolValue;
class StringValue;
class RgbValue;
class NotificationValue;
class UserCodeValue;
class ArrayValue;
class TableValue;
class TokenValue;

class IWriteStream;

class IValueVisitorConst {
public:
    virtual ~IValueVisitorConst() = default;

public:
    virtual void visit( const FloatValue& value ) = 0;

    virtual void visit( const ScalebleValue& value ) = 0;

    virtual void visit( const IntValue& value ) = 0;

    virtual void visit( const Int64Value& value ) = 0;

    virtual void visit( const BoolValue& value ) = 0;

    virtual void visit( const StringValue& value ) = 0;

    virtual void visit( const TokenValue& value ) = 0;

    virtual void visit( const RgbValue& value ) = 0;

    virtual void visit( const NotificationValue& value ) = 0;

    virtual void visit( const UserCodeValue& value ) = 0;

    virtual void visit( const ArrayValue& value ) = 0;

    virtual void visit( const TableValue& value ) = 0;
};

class IValueVisitor {
public:
    virtual ~IValueVisitor() = default;

public:
    virtual void visit( FloatValue& value ) = 0;

    virtual void visit( ScalebleValue& value ) = 0;

    virtual void visit( IntValue& value ) = 0;

    virtual void visit( Int64Value& value ) = 0;

    virtual void visit( BoolValue& value ) = 0;

    virtual void visit( StringValue& value ) = 0;

    virtual void visit( TokenValue& value ) = 0;

    virtual void visit( RgbValue& value ) = 0;

    virtual void visit( NotificationValue& value ) = 0;

    virtual void visit( UserCodeValue& value ) = 0;

    virtual void visit( ArrayValue& value ) = 0;

    virtual void visit( TableValue& value ) = 0;
};

}  // namespace common
