#pragma once

#include "EspLogTags.h"
#include "LuaBindingContext.hpp"
#include "LuaBindingExecutor.hpp"

namespace lua {
class Executor;
class IModule;
}  // namespace lua

namespace lua {
namespace module {
namespace bindings {

template <typename _BindingSignature>
class LuaBinding; /* undefined */

/**
 * Provides functional interface of a Lua binding.
 * When designing a new binding use only generic types and also ensure that
 * there're corresponding converters for your types at LuaValueConverter.hpp.
 * @tparam _Res Type of a return value to be pushed to Lua stack.
 * @tparam _ArgTypes Types of argument to be popped from Lua stack.
 *                   Number of arguments must exactly match
 */
template <typename _Res, typename... _ArgTypes>
class LuaBinding<_Res( _ArgTypes... )> {
public:
    static constexpr auto number_of_args = sizeof...( _ArgTypes );
    using callable_type = std::function<_Res( _ArgTypes... )>;

    void setContext( LuaBindingContext* newContext ) {
        m_ctx = newContext;
    }

protected:
    LuaBindingContext* m_ctx;
};

/**
 * Defines LuaBinding overloaded by the number of arguments.
 * Enable overload by inheriting this class in the following form:
 *      class MyBinding : public Overloaded<LuaBinding<bool(int)>, LuaBinding<bool(int, int)> {
 *      public:
 *          ...
 *          bool operator()(int arg1) { ... }
 *          bool operator()(int arg1, int arg2) { ... }
 *           ...
 *       };
 * The order in which overloads are specified doesn't matter.
 * Note that overload is allowed by the number of arguments, not by the types.
 */
template <typename _Binding, typename _BindingOverload>
class Overloaded : public _Binding {
public:
    using overloaded_callable_type = typename _BindingOverload::callable_type;
};

template <typename _BindingSignature, typename _OriginalBindingType>
int invokeOverloaded( LuaBindingExecutor& executor, LuaBinding<_BindingSignature>, _OriginalBindingType ) {
    return executor.returnError( "Unexpected binding signature and no overloads provided." );
}

template <typename _T1, typename _T2, typename _OriginalBindingType>
int invokeOverloaded( LuaBindingExecutor& executor, Overloaded<_T1, _T2> unused, _OriginalBindingType binding ) {
    return executor.invoke( typename Overloaded<_T1, _T2>::overloaded_callable_type{ binding } );
}

/**
 * Generic Lua binding implementation based on LuaBindingExecutor.
 * This template function is a wrapper that adapts binding functional object to a simple c-style
 * function to build BindingsMap.
 * Todo: Improve the interface of BindingsMap to enable support of functional objects directly, without this wrapper.
 * @tparam _BindingType Implementation type of a binding derived from LuaBinding.
 * @param lua Lua state pointer received from Lua when binding is invoked.
 * @return Number of arguments pushed to Lua stack.
 */
template <typename _BindingType>
int luaBinding( lua_State* lua ) {
    LuaBindingContext ctx{ lua };
    _BindingType binding;
    binding.setContext( &ctx );
    LuaBindingExecutor executor( lua );
    int resultStackSize = 0;
    if( executor.isValidStackSize( _BindingType::number_of_args ) ) {
        resultStackSize = executor.invoke( typename _BindingType::callable_type{ binding } );
    }
    else {
        resultStackSize = invokeOverloaded( executor, binding, binding );
    }
    if( auto error = ctx.getError() ) {
        return executor.returnError( error );
    }
    return resultStackSize;
}

}  // namespace bindings
}  // namespace module
}  // namespace lua
