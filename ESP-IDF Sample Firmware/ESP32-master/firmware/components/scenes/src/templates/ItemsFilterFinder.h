#pragma once

#include "TmplCommon.h"
#include "JsonParserBase.h"
#include "FileStream.h"

namespace scenes {

class ItemsFilterFinder;

class ItemsFilterFinderStateBase : public jssp::JsonParserListener<ItemsFilterFinder, ItemsFilterFinderStateBase> {
public:
    using jssp::JsonParserListener<ItemsFilterFinder, ItemsFilterFinderStateBase>::JsonParserListener;
};

class ItemsFilterFinderState_ClbkListener : public jssp::ParserState_ClbkListener<ItemsFilterFinder,
                                                                                  ItemsFilterFinderStateBase,
                                                                                  ItemsFilterFinderState_ClbkListener> {
public:
    using jssp::ParserState_ClbkListener<ItemsFilterFinder,
                                         ItemsFilterFinderStateBase,
                                         ItemsFilterFinderState_ClbkListener>::ParserState_ClbkListener;
};

class ItemsFilterFinderState_WalkObject : public jssp::ParserState_WalkObject<ItemsFilterFinder,
                                                                              ItemsFilterFinderStateBase,
                                                                              ItemsFilterFinderState_WalkObject> {
public:
    using jssp::ParserState_WalkObject<ItemsFilterFinder,
                                       ItemsFilterFinderStateBase,
                                       ItemsFilterFinderState_WalkObject>::ParserState_WalkObject;
};

class ItemsFilterFinder : public jssp::JsonParser<ItemsFilterFinderStateBase, ItemsFilterFinder> {
public:
    using BaseType = jssp::JsonParser<ItemsFilterFinderStateBase, ItemsFilterFinder>;

    enum class Mode { InWork, Completed, Failed };

    using ClbkType = std::function<Mode( const TemplateBlockData& )>;

    ItemsFilterFinder( common::IReadStream::UPtr&& stream, ClbkType clbk )
    : m_stream( std::move( stream ) )
    , m_clbk( clbk ){};

    bool find( const StringType& field );

    inline auto streamPos() const {
        return m_pos;
    }

    inline auto setCompleted() {
        m_mode = Mode::Completed;
    }

    bool reset() override;

    inline auto& blockData() {
        return m_bData;
    }

    inline void callCallback() {
        m_mode = m_clbk( blockData() );
    }

private:
    common::IReadStream::UPtr m_stream;
    ClbkType m_clbk;
    Mode m_mode = Mode::InWork;
    int m_pos = 0;
    TemplateBlockData m_bData;
};

}  // namespace scenes
