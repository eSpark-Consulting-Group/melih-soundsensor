#include <JsonListener.h>

#include <DebugUtils.h>
namespace jssp {

void JsonListenerDeviceSetting::startArray() {
    if( m_startParse ) {
        m_row = 0;
    }
}

void JsonListenerDeviceSetting::startObject() {
    if( m_startParse )
        ++m_deep;
}

void JsonListenerDeviceSetting::endObject() {
    if( m_startParse ) {
        --m_deep;
        if( m_deep ) {
            char row[4];
            sprintf( row, "%d", m_row );
            switch( tableType ) {
            case TableType::AssociationsTable:
                m_associationTable.insert( row, std::move( m_currentTable.clone() ) );
                m_currentTable.clear();
                ++m_row;
                break;
            case TableType::ConfigurationsTable:
                m_configurationsTable.insert( row, std::move( m_currentTable.clone() ) );
                m_currentTable.clear();
                ++m_row;
                break;
            default:
                break;
            }
        }
        else {
            m_startParse = false;
        }
    }
}

void JsonListenerDeviceSetting::key( String key ) {
    if( m_startParse ) {
        if( key == "associations" ) {
            tableType = TableType::AssociationsTable;
        }
        else if( key == "configurations" ) {
            tableType = TableType::ConfigurationsTable;
        }

        m_lastKey = key;
    }
    else {
        if( key == m_key ) {
            m_startParse = true;
        }
    }
}

void JsonListenerDeviceSetting::value( String value ) {
    if( m_startParse ) {
        m_currentTable.insert( m_lastKey, std::make_unique<::common::StringValue>( value ) );
    }
}

void JsonListenerDeviceSetting::value( int value ) {
    if( m_startParse ) {
        m_currentTable.insert( m_lastKey, std::make_unique<::common::IntValue>( value ) );
    }
}

void JsonListenerDeviceSetting::value( bool value ) {
    if( m_startParse ) {
        m_currentTable.insert( m_lastKey, std::make_unique<::common::BoolValue>( value ) );
    }
}

void JsonListenerDeviceSetting::value( float value ) {
    if( m_startParse ) {
        m_currentTable.insert( m_lastKey, std::make_unique<::common::FloatValue>( value ) );
    }
}

}  // namespace jssp
