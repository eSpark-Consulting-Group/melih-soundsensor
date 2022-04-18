#pragma once

#include "JsonStreamingParser.h"

#include <stack>
#include <functional>
#include <memory>

namespace jssp {

using StringType = std::string;
using _JsonParserBaseType = JsonStreamingParser;
using _JsonParserListenerBaseType = _JsonParserBaseType::JsonListener;

template <typename JsonParserType, typename SelfListenerType>
class JsonParserListener : public _JsonParserListenerBaseType {
public:
    using UPtr = std::unique_ptr<SelfListenerType>;

    using _JsonParserListenerBaseType::_JsonParserListenerBaseType;

    JsonParserListener( JsonParserType& parent )
    : m_parent( parent ) {}

    JsonParserListener( JsonParserType* parent )
    : JsonParserListener( *parent ) {}

    inline auto& parent() {
        return m_parent;
    }

    inline const auto& parent() const {
        return m_parent;
    }

    inline void setError() {
        parent().OnError();
    }

    inline bool isError() const {
        return parent().isError();
    }

private:
    JsonParserType& m_parent;
};

template <typename JsonParserListenerType, typename SelfParserType>
class JsonParser : public _JsonParserBaseType {
public:
    using StateUPtr = typename JsonParserListenerType::UPtr;

    using _JsonParserBaseType::_JsonParserBaseType;

    inline void OnError() {
        m_isSuccess = false;
    }

    inline bool isError() const {
        return !m_isSuccess;
    }

    std::tuple<JsonParserListenerType*, StateUPtr> setState( StateUPtr&& newState ) {
        m_states.top().swap( newState );
        _JsonParserBaseType::setListener( m_states.top().get() );
        return std::make_tuple( m_states.top().get(), std::move( newState ) );
    }

    template <typename State, typename... Args>
    std::tuple<State*, StateUPtr> setState( Args... args ) {
        StateUPtr stateUPtr = std::make_unique<State>( static_cast<SelfParserType*>( this ), args... );
        JsonParserListenerType* newStatePtr;
        std::tie( newStatePtr, stateUPtr ) = setState( std::move( stateUPtr ) );
        return std::make_tuple( static_cast<State*>( newStatePtr ), std::move( stateUPtr ) );
    }

    JsonParserListenerType* pushState( StateUPtr&& newState ) {
        if( !newState ) {
            return nullptr;
        }
        auto newStateRawPtr = newState.get();
        m_states.push( std::move( newState ) );
        _JsonParserBaseType::setListener( newStateRawPtr );
        return newStateRawPtr;
    }

    template <typename State, typename... Args>
    inline State* pushState( Args... args ) {
        StateUPtr stateUPtr = std::make_unique<State>( static_cast<SelfParserType*>( this ), args... );
        return static_cast<State*>( pushState( std::move( stateUPtr ) ) );
    }

    StateUPtr popState() {
        if( m_states.empty() ) {
            return nullptr;
        }
        auto oldStateUPtr = std::move( m_states.top() );
        m_states.pop();
        _JsonParserBaseType::setListener( m_states.top().get() );
        return std::move( oldStateUPtr );
    }

    bool reset() override {
        decltype( m_states ) emptyStack;
        emptyStack.swap( m_states );
        m_isSuccess = _JsonParserBaseType::reset();
        return m_isSuccess;
    }

protected:
    std::stack<StateUPtr> m_states;
    bool m_isSuccess = true;
};

template <typename ParserType, typename ListenerType>
class ParserState_SkipObject : public ListenerType {
public:
    using EndClbk = std::function<void()>;

    ParserState_SkipObject( ParserType& parent, const int depth )
    : ListenerType( parent )
    , m_depth( depth ) {}

    ParserState_SkipObject( ParserType* parent, const int depth )
    : ParserState_SkipObject( *parent, depth ) {}

    ParserState_SkipObject( ParserType& parent )
    : ParserState_SkipObject( parent, parent.depth() ) {}

    ParserState_SkipObject( ParserType* parent )
    : ParserState_SkipObject( *parent ) {}

    void key( StringType key ) override {}

    void value( StringType value ) override {}

    void value( int value ) override {}

    void value( float value ) override {}

    void value( bool value ) override {}

    void value( std::nullptr_t ) override {}

    void startArray() override {}

    void endArray() override{};

    void startObject() override {}

    void endObject() override {
        tryExitByDepth();
    };

    inline auto currKey() const {
        return ListenerType::parent().currKey();
    }

    inline auto skipDepth() const {
        return m_depth;
    }

protected:
    auto tryExitByDepth() {
        if( ListenerType::parent().depth() <= skipDepth() ) {
            return ListenerType::parent().popState();
        }
        typename ListenerType::UPtr empty;
        return std::move( empty );
    }

private:
    int m_depth;
};

template <typename ParserType, typename ListenerType>
class ParserState_SkipArray : public ParserState_SkipObject<ParserType, ListenerType> {
public:
    using _ParserState_SkipObject = ParserState_SkipObject<ParserType, ListenerType>;

    using _ParserState_SkipObject::ParserState_SkipObject;

    void endObject() override {}

    void endArray() override {
        _ParserState_SkipObject::endObject();
    };
};

template <typename ParserType, typename ListenerType, typename SelfType>
class ParserState_ClbkListener : public ListenerType {
public:
    using KeyClbk = std::function<void( SelfType&, const StringType& )>;
    using StringClbk = std::function<void( SelfType&, const StringType&, const StringType& )>;
    using IntClbk = std::function<void( SelfType&, const StringType&, int )>;
    using FloatClbk = std::function<void( SelfType&, const StringType&, float )>;
    using BoolClbk = std::function<void( SelfType&, const StringType&, bool )>;
    using NullptrClbk = std::function<void( SelfType&, const StringType& )>;

    ParserState_ClbkListener( ParserType& parent )
    : ListenerType( parent ) {}

    ParserState_ClbkListener( ParserType* parent )
    : ParserState_ClbkListener( *parent ) {}

    void key( StringType key ) override {
        m_lastKey = key;
        if( m_keyClbk ) {
            m_keyClbk( *static_cast<SelfType*>( this ), key );
        }
    }

    inline void setKeyClbk( KeyClbk keyClbk ) {
        m_keyClbk = keyClbk;
    }

    void value( StringType value ) override {
        if( m_stringValueClbk ) {
            m_stringValueClbk( *static_cast<SelfType*>( this ), m_lastKey, value );
        }
    };

    inline void setStringClbk( StringClbk stringClbk ) {
        m_stringValueClbk = stringClbk;
    }

    void value( int value ) override {
        if( m_intClbk ) {
            m_intClbk( *static_cast<SelfType*>( this ), m_lastKey, value );
        }
    };

    inline void setIntClbk( IntClbk intClbk ) {
        m_intClbk = intClbk;
    }

    void value( float value ) override {
        if( m_floatClbk ) {
            m_floatClbk( *static_cast<SelfType*>( this ), m_lastKey, value );
        }
    };

    inline void setFloatClbk( FloatClbk floatClbk ) {
        m_floatClbk = floatClbk;
    }

    void value( bool value ) override {
        if( m_boolClbk ) {
            m_boolClbk( *static_cast<SelfType*>( this ), m_lastKey, value );
        }
    };

    inline void setBoolClbk( BoolClbk boolClbk ) {
        m_boolClbk = boolClbk;
    }

    void value( std::nullptr_t ) override {
        if( m_nullptrClbk ) {
            m_nullptrClbk( *static_cast<SelfType*>( this ), m_lastKey );
        }
    };

    inline void setNullptrClbk( NullptrClbk nullptrClbk ) {
        m_nullptrClbk = nullptrClbk;
    }

    void startArray() override {
        if( m_startArrayClbk ) {
            m_startArrayClbk( *static_cast<SelfType*>( this ), m_lastKey );
        }
    };

    inline void setStartArrayClbk( NullptrClbk startArrayClbk ) {
        m_startArrayClbk = startArrayClbk;
    }

    void endArray() override {
        if( m_endArrayClbk ) {
            m_endArrayClbk( *static_cast<SelfType*>( this ), m_lastKey );
        }
    };

    inline void setEndArrayClbk( NullptrClbk endArrayClbk ) {
        m_endArrayClbk = endArrayClbk;
    }

    void startObject() override {
        if( m_startObjectClbk ) {
            m_startObjectClbk( *static_cast<SelfType*>( this ), m_lastKey );
        }
    }

    inline void setStartObjectClbk( NullptrClbk startObjectClbk ) {
        m_startObjectClbk = startObjectClbk;
    }

    void endObject() override {
        if( m_endObjectClbk ) {
            m_endObjectClbk( *static_cast<SelfType*>( this ), m_lastKey );
        }
    };

    inline void setEndObjectClbk( NullptrClbk endObjectClbk ) {
        m_endObjectClbk = endObjectClbk;
    }

private:
    StringType m_lastKey;
    KeyClbk m_keyClbk;
    StringClbk m_stringValueClbk;
    IntClbk m_intClbk;
    FloatClbk m_floatClbk;
    BoolClbk m_boolClbk;
    NullptrClbk m_nullptrClbk;
    NullptrClbk m_startArrayClbk;
    NullptrClbk m_endArrayClbk;
    NullptrClbk m_startObjectClbk;
    NullptrClbk m_endObjectClbk;
};

template <typename ParserType, typename ListenerType, typename SelfType>
class ParserState_WalkObject : public ParserState_ClbkListener<ParserType, ListenerType, SelfType> {
public:
    using _ParserState_ClbkListener = ParserState_ClbkListener<ParserType, ListenerType, SelfType>;
    using State_SkipArray = ParserState_SkipArray<ParserType, ListenerType>;
    using State_SkipObject = ParserState_SkipObject<ParserType, ListenerType>;

    using _ParserState_ClbkListener::_ParserState_ClbkListener;

    void startArray() override {
        auto& parent = _ParserState_ClbkListener::parent();
        auto state = std::make_unique<State_SkipArray>( parent );
        parent.pushState( std::move( state ) );

        _ParserState_ClbkListener::startArray();
    };

    void startObject() override {
        auto& parent = _ParserState_ClbkListener::parent();
        auto state = std::make_unique<State_SkipObject>( parent );
        parent.pushState( std::move( state ) );

        _ParserState_ClbkListener::startObject();
    }
};

}  // namespace jssp
