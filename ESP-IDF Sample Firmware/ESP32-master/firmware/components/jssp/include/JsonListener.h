
#pragma once

#include <string>
#include <cstddef>

#include "values/TableValue.h"
#include "values/StringValue.h"
#include "values/IntValue.h"
#include "values/BoolValue.h"
#include "values/FloatValue.h"

namespace jssp {

class JsonListenerBase {
public:
    using String = std::string;

    virtual ~JsonListenerBase(){};

    virtual void whitespace( char c ){};

    virtual void startDocument(){};

    virtual void key( String key ){};

    virtual void value( String value ){};

    virtual void value( int value ){};

    virtual void value( float value ){};

    virtual void value( bool value ){};

    virtual void value( std::nullptr_t ){};

    virtual void endArray(){};

    virtual void endObject(){};

    virtual void endDocument(){};

    virtual void startArray(){};

    virtual void startObject(){};

    virtual void error( int code ){};
};

class JsonListenerDeviceSetting : public JsonListenerBase {
public:
    using String = std::string;

    enum class TableType { Unknown = 0, AssociationsTable, ConfigurationsTable };

    JsonListenerDeviceSetting( String key )
    : m_key( std::move( key ) ){};

    void startObject() override;
    void endObject() override;

    void startArray() override;
    void endArray() override{};

    void key( String key ) override;

    void value( String value ) override;

    void value( int value ) override;

    void value( float value ) override;

    void value( bool value ) override;

    const common::TableValue& getConfigurationsTable() {
        return m_configurationsTable;
    }

    const common::TableValue& getAssociationTable() {
        return m_associationTable;
    }

    const common::TableValue& getParameterTable() const {
        return m_currentTable;
    }

private:
    common::TableValue m_associationTable;
    common::TableValue m_configurationsTable;
    common::TableValue m_currentTable;
    String m_key;
    String m_lastKey;
    uint8_t m_row = 0;
    uint8_t m_deep = 0;
    TableType tableType = TableType::Unknown;
    bool m_startParse = false;
};

}  // namespace jssp
