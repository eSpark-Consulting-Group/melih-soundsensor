#pragma once

#include "IValueVisitor.h"

struct lua_State;

namespace lua {

/**
 * Dispatches ::common::IValue implementations & pushes the underlying value to Lua stack.
 * The visitor does not explicitly set Lua stack size.
 */
class ValueToLuaGenerator final : public ::common::IValueVisitorConst {
public:
    explicit ValueToLuaGenerator(::lua_State* luaState );

    void visit( const ::common::FloatValue& value ) override;
    void visit( const ::common::ScalebleValue& value ) override;
    void visit( const ::common::BoolValue& value ) override;
    void visit( const ::common::IntValue& value ) override;
    void visit( const ::common::Int64Value& value ) override;
    void visit( const ::common::StringValue& value ) override;
    void visit( const ::common::TokenValue& value ) override;
    void visit( const ::common::RgbValue& value ) override;
    void visit( const ::common::NotificationValue& value ) override;
    void visit( const ::common::ArrayValue& value ) override;
    void visit( const ::common::TableValue& value ) override;
    void visit( const ::common::UserCodeValue& value ) override;

private:
    ::lua_State* m_luaState;
};

}  // namespace lua
