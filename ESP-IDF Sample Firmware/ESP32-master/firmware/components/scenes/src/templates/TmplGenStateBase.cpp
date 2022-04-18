#include "TmplGenStateBase.h"
#include "TmplGenerator.h"

#include "DebugUtils.h"

namespace scenes {

const TemplateBlockData& TmplGenState_Base::getData() const {
    return parent().getData();
}

common::JsonStreamGenerator& TmplGenState_Base::getGen() {
    return parent().getGen();
}

const gateways::ItemInfo& TmplGenState_Base::getItemInfo() const {
    return reinterpret_cast<const TemplatesGenerator&>( parent() ).getItemInfo();
}

TmplGenState_WriteObject::TmplGenState_WriteObject( TemplatesGenerator* parent, const int depth )
: TmplGenState_Base( parent )
, m_depth( depth ) {}

void TmplGenState_WriteObject::endObject() {
    TmplGenState_Base::endObject();
    if( parent().depth() > m_depth ) {
        return;
    }
    parent().popState();
};

bool TmplGenState_WriteData::isItemIdBlock() const {
    return getData().itemIdCoord().isWithin( parent().getPos() );
}

bool TmplGenState_WriteData::isTokenValueOptionsBlock() const {
    return getData().itemTokenCoord().isWithin( parent().getPos() );
}

bool TmplGenState_WriteData::isDictIdBlock() const {
    return getData().dictBlockCoord().isWithin( parent().getPos() );
}

void TmplGenState_WriteData::key( StringType key ) {
    if( tokens::ItemFilter == key || tokens::DeviceFilter == key ) {
        if( nullptr == parent().pushState<TmplGenState_SkipObject>( parent().depth() + 1 ) ) {
            setError();
        }
        return;
    }
    TmplGenState_Base::key( key );
}

void TmplGenState_WriteData::startObject() {
    TmplGenState_Base::startObject();
    if( isError() ) {
        return;
    }

    if( isItemIdBlock() ) {
        if( ( nullptr == parent().pushState<TmplGenState_SkipObject>() ) ||
            !getGen().writeProperty( tokens::Name, tokens::Item ) ||
            !getGen().writeProperty( tokens::Type, tokens::Item ) ||
            !getGen().writeProperty( tokens::Value, getItemInfo().getId() ) || !getGen().writeClosingSymbol() ) {
            setError();
        }
    }
    else if( isTokenValueOptionsBlock() ) {
        const auto& itemInfo = parent().getItemInfo();
        if( itemInfo.isEnum() ) {
            if( nullptr == parent().pushState<TmplGenState_WriteTokenValueOption>( parent().depth() ) ) {
                setError();
            }
        }
        else {
            FUNC_LOGW( "Value of item<id=%s, name=%s> is not token type. Because of it we just create a scene block "
                       "with all possible choises.",
                       itemInfo.getId().c_str(),
                       itemInfo.getName().c_str() );
        }
    }
    else if( isDictIdBlock() ) {
        if( nullptr == parent().pushState<TmplGenState_ReadDictionaryId>( parent().depth() ) ) {
            setError();
        }
        return;
    }
};

TmplGenState_WriteTokenValueOption::TmplGenState_WriteTokenValueOption( TemplatesGenerator* parent, const int depth )
: TmplGenState_Base( parent )
, m_depth( depth ) {}

void TmplGenState_WriteTokenValueOption::key( StringType key ) {
    m_currKey = key;
    if( tokens::Value == key ) {
        return;
    }

    TmplGenState_Base::key( key );
}

void TmplGenState_WriteTokenValueOption::value( StringType value ) {
    if( tokens::Value == m_currKey ) {
        m_tokenValue = value;
        return;
    }
    TmplGenState_Base::value( value );
}

void TmplGenState_WriteTokenValueOption::value( int value ) {
    if( tokens::Value == m_currKey ) {
        setError();
        return;
    }
    TmplGenState_Base::value( value );
}

void TmplGenState_WriteTokenValueOption::value( float value ) {
    if( tokens::Value == m_currKey ) {
        setError();
        return;
    }
    TmplGenState_Base::value( value );
}

void TmplGenState_WriteTokenValueOption::value( bool value ) {
    if( tokens::Value == m_currKey ) {
        setError();
        return;
    }
    TmplGenState_Base::value( value );
}

void TmplGenState_WriteTokenValueOption::value( std::nullptr_t ) {
    if( tokens::Value == m_currKey ) {
        setError();
        return;
    }
    TmplGenState_Base::value( nullptr );
}

void TmplGenState_WriteTokenValueOption::startArray() {
    TmplGenState_Base::startArray();

    if( m_currKey == tokens::Options ) {
        auto tokenClbk = [this]( const StringType& token ) { m_tokenValues.emplace_back( token ); };

        if( nullptr ==
            parent().pushState<TmplGenState_WriteTokenValueFilteredOptionEntry>( parent().depth(), tokenClbk ) ) {
            setError();
        }
    }
}

void TmplGenState_WriteTokenValueOption::endObject() {
    if( parent().depth() > m_depth ) {
        TmplGenState_Base::endObject();
        return;
    }

    if( !m_tokenValues.empty() ) {
        bool isValue = false;
        for( const auto value : m_tokenValues ) {
            if( m_tokenValue == value ) {
                getGen().writeProperty( tokens::Value, value );
                isValue = true;
                break;
            }
        }

        if( !isValue ) {
            getGen().writeProperty( tokens::Value, m_tokenValues.front() );
        }
    }
    else {
        getGen().writeProperty( tokens::Value, m_tokenValue );
    }

    TmplGenState_Base::endObject();
    parent().popState();
};

TmplGenState_WriteTokenValueFilteredOptionEntry::TmplGenState_WriteTokenValueFilteredOptionEntry(
    TemplatesGenerator* parent,
    const int depth,
    TokenClbk tokenClbk )
: TmplGenState_SkipArray( parent, depth )
, m_tokenClbk( tokenClbk ) {}

void TmplGenState_WriteTokenValueFilteredOptionEntry::startObject() {
    TmplGenState_SkipArray::startObject();
    if( parent().depth() == skipDepth() + 1 ) {
        parent().storeState( m_stateData );
    }
}

void TmplGenState_WriteTokenValueFilteredOptionEntry::value( StringType value ) {
    TmplGenState_SkipArray::value( value );
    const auto& itemInfo = parent().getItemInfo();
    const auto enumsCount = itemInfo.getEnumItemsCount();
    for( auto idx = 0; idx < enumsCount; ++idx ) {
        const auto& enumVal = itemInfo.getEnumValue( idx );
        if( enumVal == value ) {
            parent().restoreState( m_stateData );
            if( nullptr == parent().pushState<TmplGenState_WriteObject>( skipDepth() + 1 ) ) {
                setError();
            }
            getGen().writeOpeningObject();
            if( m_tokenClbk ) {
                m_tokenClbk( value );
            }
            else {
                FUNC_LOGI( "Found proper token = %s", value.c_str() );
            }
            return;
        }
    }

    if( nullptr == parent().pushState<TmplGenState_SkipObject>( skipDepth() + 1 ) ) {
        setError();
    }
};

void TmplGenState_WriteTokenValueFilteredOptionEntry::endArray() {
    getGen().writeClosingSymbol();
    TmplGenState_SkipArray::endArray();
};

void TmplGenState_ReadDictionaryId::value( StringType value ) {
    TmplGenState_SkipObject::value( value );
    if( tokens::Item == currKey() ) {
        m_dictIdItemName = value;
    }
    else if( tokens::ResultField == currKey() ) {
        m_dictIdResultField = value;
    }
    else if( tokens::Value == currKey() ) {
        m_dictIdValue = value;
    }
}

void TmplGenState_ReadDictionaryId::endObject() {
    auto state = tryExitByDepth();
    if( !state ) {
        return;
    }
    else if( !m_dictIdItemName.empty() && !m_dictIdResultField.empty() ) {
        StringType itemId;
        const auto& itemInfoList = parent().getItemsInfoList();
        for( const auto& itemInfo : itemInfoList ) {
            if( m_dictIdItemName == itemInfo.getName() ) {
                itemId = itemInfo.getId();
            }
        }

        bool isSuccess = getGen().writeProperty( tokens::Name, tokens::User ) &&
                         getGen().writeProperty( tokens::Type, tokens::DictionaryId ) &&
                         getGen().writeProperty( tokens::Value, m_dictIdValue );
        if( itemId.empty() ) {
            FUNC_LOGE( "Can't find item with name=%s for device=%s",
                       m_dictIdItemName.c_str(),
                       parent().getDeviceInfo().getName() );
        }
        else if( isSuccess ) {
            isSuccess = isSuccess && getGen().writePropertyKey( tokens::OptionItemValue );
            isSuccess = isSuccess && getGen().writeOpeningObject();
            isSuccess = isSuccess && getGen().writeProperty( m_dictIdResultField.c_str(), itemId );
            isSuccess = isSuccess && getGen().writeClosingSymbol();
        }
        isSuccess&& getGen().writeClosingSymbol();
    }
}

}  // namespace scenes
