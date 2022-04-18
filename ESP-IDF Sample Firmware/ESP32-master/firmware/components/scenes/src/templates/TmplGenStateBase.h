#pragma once

#include "TmplCommon.h"
#include "JsonParserBase.h"

#include "JsonStreamGenerator.h"

#include <functional>

namespace gateways {

class ItemInfo;

}  // namespace gateways

namespace scenes {

struct StateData {
    int m_pos;
    jssp::ParserState m_parserState;
};

class TemplatesGenerator;

class TmplGenState_Base : public jssp::JsonParserListener<TemplatesGenerator, TmplGenState_Base> {
public:
    using jssp::JsonParserListener<TemplatesGenerator, TmplGenState_Base>::JsonParserListener;

    const TemplateBlockData& getData() const;

    common::JsonStreamGenerator& getGen();

    const gateways::ItemInfo& getItemInfo() const;

    void key( StringType key ) override {
        if( !getGen().writePropertyKey( key.c_str() ) ) {
            setError();
        }
    }

    void value( StringType value ) override {
        if( !getGen().writeProperty( nullptr, value ) ) {
            setError();
        }
    };

    void value( int value ) override {
        if( !getGen().writeProperty( nullptr, value ) ) {
            setError();
        }
    };

    void value( float value ) override {
        if( !getGen().writeProperty( nullptr, value ) ) {
            setError();
        }
    };

    void value( bool value ) override {
        if( !getGen().writeProperty( nullptr, value ) ) {
            setError();
        }
    };

    void value( std::nullptr_t ) override {
        if( !getGen().writeProperty( nullptr, nullptr ) ) {
            setError();
        }
    };

    void startArray() override {
        if( !getGen().writeOpeningArray() ) {
            setError();
        }
    };

    void endArray() override {
        if( !getGen().writeClosingSymbol() ) {
            setError();
        }
    };

    void startObject() override {
        if( !getGen().writeOpeningObject() ) {
            setError();
        }
    }

    void endObject() override {
        if( !getGen().writeClosingSymbol() ) {
            setError();
        }
    };
};

using TmplGenState_SkipArray = jssp::ParserState_SkipArray<TemplatesGenerator, TmplGenState_Base>;

using TmplGenState_SkipObject = jssp::ParserState_SkipObject<TemplatesGenerator, TmplGenState_Base>;

class TmplGenState_WriteObject : public TmplGenState_Base {
protected:
    int m_depth;

public:
    TmplGenState_WriteObject( TemplatesGenerator* parent, const int depth );

    void endObject() override;
};

class TmplGenState_WriteData : public TmplGenState_Base {
public:
    using TmplGenState_Base::TmplGenState_Base;

    void key( StringType key ) override;

    void startObject() override;

    inline bool isItemIdBlock() const;

    inline bool isTokenValueOptionsBlock() const;

    inline bool isDictIdBlock() const;
};

class TmplGenState_WriteTokenValueOption : public TmplGenState_Base {
    int m_depth;
    StringType m_currKey;
    StringType m_tokenValue;
    std::list<StringType> m_tokenValues;

public:
    TmplGenState_WriteTokenValueOption( TemplatesGenerator* parent, const int depth );

    void key( StringType key ) override;

    void value( StringType value ) override;

    void value( int value ) override;

    void value( float value ) override;

    void value( bool value ) override;

    void value( std::nullptr_t ) override;

    void startArray() override;

    void endObject() override;
};

class TmplGenState_WriteTokenValueFilteredOptionEntry : public TmplGenState_SkipArray {
public:
    using TokenClbk = std::function<void( const StringType& )>;

    TmplGenState_WriteTokenValueFilteredOptionEntry( TemplatesGenerator* parent, const int depth, TokenClbk tokenClbk );

    void startObject() override;

    void value( StringType value ) override;

    void endArray() override;

private:
    StateData m_stateData;
    TokenClbk m_tokenClbk;
};

class TmplGenState_ReadDictionaryId final : public TmplGenState_SkipObject {
    StringType m_dictIdItemName, m_dictIdResultField, m_dictIdValue;

public:
    using TmplGenState_SkipObject::TmplGenState_SkipObject;

    void value( StringType value ) override;

    void endObject() override;
};

}  // namespace scenes
