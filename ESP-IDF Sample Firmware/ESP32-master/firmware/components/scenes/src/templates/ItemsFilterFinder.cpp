#include "ItemsFilterFinder.h"

namespace scenes {

namespace {

void popItemsFilterFinderState_WalkObject( ItemsFilterFinderState_WalkObject& state, const StringType& key ) {
    state.parent().popState();
}

void onDevFilter_ReadProhibitedCategoryObjectArray( ItemsFilterFinderState_WalkObject& state,
                                                    const StringType& category ) {
    ItemsFilterFinderState_WalkObject* newState;
    std::tie( newState, std::ignore ) = state.parent().setState<ItemsFilterFinderState_WalkObject>();
    newState->setStringClbk(
        [&category]( ItemsFilterFinderState_WalkObject& state, const StringType& key, const StringType& value ) {
            auto& prohibDev = state.parent().blockData().prohibitedDevices();
            prohibDev[category].emplace( value );
        } );
    newState->setEndArrayClbk( popItemsFilterFinderState_WalkObject );
}

void onDevFilter_ReadProhibitedCategoryObject( ItemsFilterFinderState_WalkObject& state, const StringType& key ) {
    ItemsFilterFinderState_WalkObject* newState;
    std::tie( newState, std::ignore ) = state.parent().setState<ItemsFilterFinderState_WalkObject>();
    newState->setStartArrayClbk( onDevFilter_ReadProhibitedCategoryObjectArray );
    newState->setEndObjectClbk( popItemsFilterFinderState_WalkObject );
};

void onDevFilter_ReadProhibitionArray( ItemsFilterFinderState_WalkObject& state, const StringType& key ) {
    if( tokens::Category != key ) {
        return;
    }

    ItemsFilterFinderState_WalkObject* newState;
    std::tie( newState, std::ignore ) = state.parent().setState<ItemsFilterFinderState_WalkObject>();
    newState->setStartObjectClbk( onDevFilter_ReadProhibitedCategoryObject );
    newState->setEndArrayClbk( popItemsFilterFinderState_WalkObject );
};

void onDevFilter_ReadAllowedCategoryObjectArray( ItemsFilterFinderState_WalkObject& state,
                                                 const StringType& category ) {
    ItemsFilterFinderState_WalkObject* newState;
    std::tie( newState, std::ignore ) = state.parent().setState<ItemsFilterFinderState_WalkObject>();
    newState->setStringClbk(
        [&category]( ItemsFilterFinderState_WalkObject& state, const StringType& key, const StringType& value ) {
            auto& allowDev = state.parent().blockData().allowedDevices();
            allowDev[category].emplace( value );
        } );
    newState->setEndArrayClbk( popItemsFilterFinderState_WalkObject );
}

void onDevFilter_ReadAllowedCategoryObject( ItemsFilterFinderState_WalkObject& state, const StringType& key ) {
    ItemsFilterFinderState_WalkObject* newState;
    std::tie( newState, std::ignore ) = state.parent().setState<ItemsFilterFinderState_WalkObject>();
    newState->setStartArrayClbk( onDevFilter_ReadAllowedCategoryObjectArray );
    newState->setEndObjectClbk( popItemsFilterFinderState_WalkObject );
};

void onDevFilter_StartRootArray( ItemsFilterFinderState_WalkObject& state, const StringType& key ) {
    if( tokens::Category != key ) {
        return;
    }

    ItemsFilterFinderState_WalkObject* newState;
    std::tie( newState, std::ignore ) = state.parent().setState<ItemsFilterFinderState_WalkObject>();
    newState->setStartObjectClbk( onDevFilter_ReadAllowedCategoryObject );
    newState->setEndArrayClbk( popItemsFilterFinderState_WalkObject );
}

void onDevFilter_StartRootObject( ItemsFilterFinderState_WalkObject& state, const StringType& key ) {
    if( tokens::Not != key ) {
        return;
    }

    ItemsFilterFinderState_WalkObject* newState;
    std::tie( newState, std::ignore ) = state.parent().setState<ItemsFilterFinderState_WalkObject>();
    newState->setStartArrayClbk( onDevFilter_ReadProhibitionArray );
    newState->setEndObjectClbk( popItemsFilterFinderState_WalkObject );
}

void onBlockValueObjectStringValue( ItemsFilterFinderState_ClbkListener& state,
                                    const StringType& key,
                                    const StringType& value ) {
    if( tokens::Names == key ) {
        state.parent().blockData().tokens().emplace( value );
    }
};

void onSceneBlockValueArray( ItemsFilterFinderState_WalkObject& state, const StringType& key ) {
    if( tokens::Fields != key ) {
        return;
    }

    ItemsFilterFinderState_ClbkListener* newState;
    std::tie( newState, std::ignore ) = state.parent().setState<ItemsFilterFinderState_ClbkListener>();
    auto blockCoord = std::make_shared<BlockCoord>();
    auto blockType = std::make_shared<StringType>();
    auto objDepth = state.parent().depth() + 1;

    newState->setStartObjectClbk(
        [blockCoord, blockType, objDepth]( ItemsFilterFinderState_ClbkListener& state, const StringType& key ) {
            if( state.parent().depth() == objDepth ) {
                blockCoord->setFrom( state.parent().streamPos() );
            }
        } );

    newState->setStringClbk( [blockCoord, blockType, objDepth]( ItemsFilterFinderState_ClbkListener& state,
                                                                const StringType& key,
                                                                const StringType& value ) {
        if( state.parent().depth() == objDepth && tokens::Type == key ) {
            *blockType = value;
        }
    } );

    newState->setEndObjectClbk(
        [blockCoord, blockType, objDepth]( ItemsFilterFinderState_ClbkListener& state, const StringType& key ) {
            if( state.parent().depth() > objDepth ) {
                return;
            }

            blockCoord->setTo( state.parent().streamPos() );

            if( tokens::Item == *blockType ) {
                state.parent().blockData().itemIdCoord().setFrom( blockCoord->from() );
                state.parent().blockData().itemIdCoord().setTo( blockCoord->to() );
            }
            else if( tokens::Token == *blockType ) {
                state.parent().blockData().itemTokenCoord().setFrom( blockCoord->from() );
                state.parent().blockData().itemTokenCoord().setTo( blockCoord->to() );
            }
            else if( tokens::DictionaryId == *blockType ) {
                state.parent().blockData().dictBlockCoord().setFrom( blockCoord->from() );
                state.parent().blockData().dictBlockCoord().setTo( blockCoord->to() );
            }
        } );

    newState->setEndArrayClbk(
        [arrayDepth = state.parent().depth()]( ItemsFilterFinderState_ClbkListener& state, const StringType& key ) {
            if( state.parent().depth() <= arrayDepth ) {
                state.parent().popState();
            }
        } );
};

void onSceneBlockValueObject( ItemsFilterFinderState_WalkObject& state, const StringType& key ) {
    if( tokens::ItemFilter == key ) {
        ItemsFilterFinderState_ClbkListener* newState;
        std::tie( newState, std::ignore ) = state.parent().setState<ItemsFilterFinderState_ClbkListener>();
        newState->setStringClbk( onBlockValueObjectStringValue );
        newState->setEndObjectClbk(
            [depth = state.parent().depth()]( ItemsFilterFinderState_ClbkListener& state, const StringType& key ) {
                if( state.parent().depth() <= depth ) {
                    state.parent().popState();
                }
            } );
    }
    else if( tokens::DeviceFilter == key ) {
        ItemsFilterFinderState_WalkObject* newState;
        std::tie( newState, std::ignore ) = state.parent().setState<ItemsFilterFinderState_WalkObject>();
        newState->setStartObjectClbk( onDevFilter_StartRootObject );
        newState->setStartArrayClbk( onDevFilter_StartRootArray );
        newState->setEndObjectClbk( popItemsFilterFinderState_WalkObject );
    }
};

void onEndSceneBlockObject( ItemsFilterFinderState_WalkObject& state, const StringType& key ) {
    // FUNC_LOGI( "*******************************------< End Block pos=%d, depth=%d",
    //           state.parent().streamPos(),
    //           state.parent().depth() );
    state.parent().blockData().blockCoord().setTo( state.parent().streamPos() + 1 );
    state.parent().callCallback();
    state.parent().popState();
};

void onStartSceneBlockObject( ItemsFilterFinderState_WalkObject& state, const StringType& key ) {
    // FUNC_LOGI( "*******************************------> Start Block pos=%d, depth=%d",
    //           state.parent().streamPos(),
    //           state.parent().depth() );
    state.parent().blockData().reset();
    state.parent().blockData().blockCoord().setFrom( state.parent().streamPos() );

    ItemsFilterFinderState_WalkObject* newState;
    std::tie( newState, std::ignore ) = state.parent().setState<ItemsFilterFinderState_WalkObject>();
    newState->setStartObjectClbk( onSceneBlockValueObject );
    newState->setStartArrayClbk( onSceneBlockValueArray );
    newState->setEndObjectClbk( onEndSceneBlockObject );
};

void onEndSceneBlocksArray( ItemsFilterFinderState_WalkObject& state, const StringType& key ) {
    state.parent().setCompleted();
};

}  // namespace

bool ItemsFilterFinder::find( const StringType& field ) {
    auto initialState = pushState<ItemsFilterFinderState_ClbkListener>();
    initialState->setStartObjectClbk( [&field]( ItemsFilterFinderState_ClbkListener& state, const StringType& key ) {
        auto onStartRootArrayClbk = [&field]( ItemsFilterFinderState_WalkObject& state, const StringType& key ) {
            if( key != field ) {
                return;
            }

            ItemsFilterFinderState_WalkObject* newState;
            std::tie( newState, std::ignore ) = state.parent().setState<ItemsFilterFinderState_WalkObject>();
            newState->setStartObjectClbk( onStartSceneBlockObject );
            newState->setEndArrayClbk( onEndSceneBlocksArray );
        };

        ItemsFilterFinderState_WalkObject* newState;
        std::tie( newState, std::ignore ) = state.parent().setState<ItemsFilterFinderState_WalkObject>();
        newState->setStartArrayClbk( onStartRootArrayClbk );
    } );

    char ch;
    while( m_stream->read( &ch, 1 ) ) {
        parse( ch );
        if( isError() || Mode::Failed == m_mode ) {
            return false;
        }
        if( Mode::InWork != m_mode ) {
            break;
        }
        ++m_pos;
    }

    return true;
}

bool ItemsFilterFinder::reset() {
    if( !BaseType::reset() ) {
        OnError();
    }
    m_mode = Mode::InWork;
    m_pos = 0;
    if( !m_stream->reset( 0 ) ) {
        OnError();
    }
    return !isError();
}

}  // namespace scenes
