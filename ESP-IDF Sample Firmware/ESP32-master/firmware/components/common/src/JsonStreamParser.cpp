
#include <JsonStreamParser.h>

#include <values/StringValue.h>
#include <values/IntValue.h>
#include <values/BoolValue.h>
#include <values/FloatValue.h>

#include <stdio.h>
#include <cstdlib>
#include <string.h>

#include <cinttypes>

#include <DebugUtils.h>

namespace common {
namespace {
constexpr char TrueBoolValue[] = "true";
constexpr char FalseBoolValue[] = "false";
}  // namespace

JsonStreamParser::JsonStreamParser( char* buffer, std::size_t size )
: m_maxBufferSize( size )
, m_buffer( buffer ) {}

bool JsonStreamParser::isQuotedTokenProcessing() const {
    return m_stateFlags & Flags::QuotedToken;
}

bool JsonStreamParser::isValueProcessing() const {
    return m_stateFlags & Flags::Value;
}

bool JsonStreamParser::isNotQuotedTokenProcessing() const {
    return m_stateFlags & Flags::NotQuotedToken;
}

bool JsonStreamParser::isFloatTokenProcessing() const {
    return m_stateFlags & Flags::FloatToken;
}

void JsonStreamParser::setTokenTruncated() {
    m_stateFlags |= Flags::TokenTruncated;
}

void JsonStreamParser::enableQuotedTokenProcessing() {
    m_stateFlags |= Flags::QuotedToken;
}

void JsonStreamParser::enableFloatTokenProcessing() {
    m_stateFlags |= Flags::FloatToken;
}

void JsonStreamParser::enableNotQuotedTokenProcessing() {
    m_stateFlags |= Flags::NotQuotedToken;
}

void JsonStreamParser::enableValueTokenProcessing() {
    m_stateFlags |= Flags::Value;
}

void JsonStreamParser::disableValueTokenProcessing() {
    m_stateFlags ^= Flags::Value;
}

void JsonStreamParser::addCharToToken( char value ) {
    if( isActiveBlockNeededToParse() ) {
        if( m_activeTokenSize < m_maxBufferSize ) {
            m_buffer[m_activeTokenSize++] = value;
        }
        else if( m_activeTokenSize == m_maxBufferSize && onLongToken() ) {
            m_activeTokenSize = 0;
            m_buffer[m_activeTokenSize++] = value;
        }
        else {
            setTokenTruncated();
        }
    }
}

bool JsonStreamParser::isSame( const char* token, std::size_t tokenSize ) const {
    return ( tokenSize == m_activeTokenSize ) && ( strncmp( m_buffer, token, m_activeTokenSize ) == 0 );
}

bool JsonStreamParser::isSameInOneCase( const char* token, std::size_t tokenSize ) const {
    return ( tokenSize == m_activeTokenSize ) && ( strncasecmp( m_buffer, token, m_activeTokenSize ) == 0 );
}

bool JsonStreamParser::isSame( const char* token ) const {
    return strcmp( m_buffer, token ) == 0;
}

bool JsonStreamParser::isSameInOneCase( const char* token ) const {
    return strcasecmp( m_buffer, token ) == 0;
}

bool JsonStreamParser::read( IValue*& value ) const {
    FUNC_LOGW( "To value: %s", getTokenValue() );
    if( isNotQuotedTokenProcessing() ) {
        if( isFloatTokenProcessing() ) {
            float floatValue = 0;
            if( read( floatValue ) ) {
                value = new FloatValue( floatValue );
                return true;
            }
        }

        bool boolValue = false;
        if( read( boolValue ) ) {
            value = new BoolValue( boolValue );
            return true;
        }

        int intValue = 0;
        if( read( intValue ) ) {
            value = new IntValue( intValue );
            return true;
        }
    }
    else {
        value = new StringValue( getTokenValue(), getTokenSize() );
        return true;
    }

    return false;
}

bool JsonStreamParser::read( bool& value ) const {
    if( isNotQuotedTokenProcessing() && !isFloatTokenProcessing() ) {
        if( isSameInOneCase( TrueBoolValue, sizeof( TrueBoolValue ) ) ) {
            value = true;
            return true;
        }
        else if( isSameInOneCase( FalseBoolValue, sizeof( FalseBoolValue ) ) ) {
            value = false;
            return true;
        }
    }

    return false;
}

bool JsonStreamParser::read( float& value ) const {
    return 1 == sscanf( getTokenValue(), "%f", &value );
}

bool JsonStreamParser::read( int& value ) const {
    return 1 == sscanf( getTokenValue(), "%d", &value );
}

bool JsonStreamParser::read( uint16_t& value ) const {
    return 1 == sscanf( getTokenValue(), "%" SCNu16, &value );
}

bool JsonStreamParser::readHex( uint32_t& value ) const {
    if( 1 != sscanf( getTokenValue(), "%" PRIX32, &value ) ) {
        return ( 1 == sscanf( getTokenValue(), "%" PRIx32, &value ) );
    }

    return true;
}

unsigned int JsonStreamParser::getCurrentDepth() const {
    return m_depth;
}

const char* JsonStreamParser::getTokenValue() const {
    return m_buffer;
}

std::size_t JsonStreamParser::getTokenSize() const {
    return m_activeTokenSize;
}

bool JsonStreamParser::isActiveBlockNeededToParse() const {
    return ( m_ignoreLevel == 0 ) || ( m_ignoreLevel > m_depth );
}

void JsonStreamParser::ignoreAllNestedBlocks() {
    m_ignoreLevel = m_depth;
}

void JsonStreamParser::onSymbolParsed( char /*symbol*/ ) {}

bool JsonStreamParser::onLongToken() {
    // Keep trancated value by default
    return false;
}

bool JsonStreamParser::parse( const char* buf, size_t len ) {
    for( size_t idx = 0; idx < len; ++idx, ++buf ) {
        onSymbolParsed( *buf );

        switch( *buf ) {
        case '{':
        case '[':
            if( isQuotedTokenProcessing() ) {
                addCharToToken( *buf );
                break;
            }

            if( isNotQuotedTokenProcessing() ) {
                if( !processActiveTokenAndResetState() ) {
                    return false;
                }
            }
            else if( isValueProcessing() ) {
                disableValueTokenProcessing();
            }

            goDeeper();
            break;

        case '}':
        case ']':
            if( isQuotedTokenProcessing() ) {
                addCharToToken( *buf );
                break;
            }

            if( isNotQuotedTokenProcessing() ) {
                if( !processActiveTokenAndResetState() ) {
                    return false;
                }
            }

            goHigher();
            break;

        case '\'':
        case '"':
            if( isQuotedTokenProcessing() ) {
                if( !processActiveTokenAndResetState() ) {
                    return false;
                }
            }
            else {
                enableQuotedTokenProcessing();
            }
            break;

        case ':':
            if( isQuotedTokenProcessing() ) {
                addCharToToken( *buf );
                break;
            }
            else if( isNotQuotedTokenProcessing() ) {
                if( !processActiveTokenAndResetState() ) {
                    return false;
                }
            }

            enableValueTokenProcessing();
            break;

        case ',':
            if( isQuotedTokenProcessing() ) {
                addCharToToken( *buf );
                break;
            }
            else if( isNotQuotedTokenProcessing() ) {
                if( !processActiveTokenAndResetState() ) {
                    return false;
                }
            }
            break;

        case '.':
            if( isQuotedTokenProcessing() ) {
                addCharToToken( *buf );
            }
            else if( isNotQuotedTokenProcessing() ) {
                addCharToToken( *buf );
                enableFloatTokenProcessing();
            }
            break;

        case ' ':
        case '\t':
        case '\r':
        case '\n':
            if( isQuotedTokenProcessing() ) {
                addCharToToken( *buf );
            }
            else if( isNotQuotedTokenProcessing() ) {
                if( !processActiveTokenAndResetState() ) {
                    return false;
                }
            }
            break;

        default:
            if( isQuotedTokenProcessing() ) {
                addCharToToken( *buf );
            }
            else {
                enableNotQuotedTokenProcessing();
                addCharToToken( *buf );
            }
            break;
        }
    }

    return true;
}

void JsonStreamParser::resetTokenProcessing() {
    m_activeTokenSize = 0;
    m_stateFlags = Flags::Empty;
}

bool JsonStreamParser::processActiveTokenAndResetState() {
    auto result = true;

    if( isActiveBlockNeededToParse() ) {
        if( m_activeTokenSize < m_maxBufferSize ) {
            m_buffer[m_activeTokenSize++] = '\0';
        }

        result = processActiveToken();
    }

    resetTokenProcessing();
    return result;
}

void JsonStreamParser::goHigher() {
    if( m_depth > 0 ) {
        --m_depth;
    }

    if( m_ignoreLevel == 0 ) {
        onStepOut();
    }
    else if( isActiveBlockNeededToParse() ) {
        m_ignoreLevel = 0;
        onStepOut();
    }
}

void JsonStreamParser::goDeeper() {
    ++m_depth;

    if( isActiveBlockNeededToParse() ) {
        onStepIn();
    }
}

bool JsonStreamParser::onStepIn() {
    return true;
}

bool JsonStreamParser::onStepOut() {
    return true;
}

}  // namespace common
