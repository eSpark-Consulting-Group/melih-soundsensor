#include "TmplGenerator.h"

#include "plugin_manager.h"

namespace scenes {

bool TemplatesGenerator::generate( const gateways::ItemInfo& itemInfo, const TemplateBlockData& data ) {
    if( !reset() || !pushState<TmplGenState_WriteData>() ) {
        return false;
    }

    m_itemInfo = &itemInfo;
    m_data = &data;

    m_isSuccess = m_stream->reset( data.blockCoord().from() );
    if( !m_isSuccess ) {
        return false;
    }
    m_pos = data.blockCoord().from();
    while( m_pos != data.blockCoord().to() ) {
        char ch;
        m_isSuccess = ( 1 == m_stream->read( &ch, 1 ) );
        ++m_pos;
        if( !m_isSuccess ) {
            return false;
        }

        parse( ch );
        if( !m_isSuccess ) {
            return false;
        }

        if( m_stateToRestore ) {
            if( m_stream->reset( m_stateToRestore->m_pos ) ) {
                m_pos = m_stateToRestore->m_pos;
                BaseType::restoreState( m_stateToRestore->m_parserState );
                m_stateToRestore = nullptr;
                continue;
            }
            m_isSuccess = false;
            return false;
        }
    }

    return true;
}

TemplatesGenerator::TemplatesGenerator( common::JsonStreamGenerator& gen,
                                        common::IReadStream::UPtr&& stream,
                                        const gateways::BaseDeviceInfo& deviceInfo,
                                        const std::list<gateways::ItemInfo>& itemInfoList )
: m_gen( gen )
, m_stream( std::move( stream ) )
, m_deviceInfo( deviceInfo )
, m_itemInfoList( itemInfoList ){};

bool TemplatesGenerator::reset() {
    if( !BaseType::reset() ) {
        OnError();
    }

    m_stateToRestore = nullptr;

    return !isError();
}

}  // namespace scenes
