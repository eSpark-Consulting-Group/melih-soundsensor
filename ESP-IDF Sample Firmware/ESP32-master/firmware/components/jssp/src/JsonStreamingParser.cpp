#include "JsonStreamingParser.h"
#include "IReadStream.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "DebugUtils.h"

namespace jssp {

JsonStreamingParser::JsonStreamingParser() {
    reset();
}

bool JsonStreamingParser::reset() {
    m_state = STATE_START_DOCUMENT;
    m_stackPos = 0;
    m_bufferPos = 0;
    m_depth = 0;
    m_unicodeEscapeBufferPos = 0;
    m_unicodeBufferPos = 0;
    m_keysStack = KeysStackType();
    return true;
}

void JsonStreamingParser::storeState( ParserState& state ) const {
    state.m_state = m_state;
    std::memcpy( state.stack, m_stack, STACK_MAX_SIZE );
    state.stackPos = m_stackPos;
    state.depth = m_depth;
    state.m_keysStack = m_keysStack;
}

void JsonStreamingParser::restoreState( const ParserState& state ) {
    m_state = state.m_state;
    std::memcpy( m_stack, state.stack, STACK_MAX_SIZE );
    m_stackPos = state.stackPos;
    m_depth = state.depth;
    m_keysStack = std::move( state.m_keysStack );
}

void JsonStreamingParser::setListener( JsonListener* listener ) {
    m_myListener = listener;
}

void JsonStreamingParser::parse( char c ) {
    // System.out.print(c);
    // valid whitespace characters in JSON (from RFC4627 for JSON) include:
    // space, horizontal tab, line feed or new line, and carriage return.
    // thanks:
    // http://stackoverflow.com/questions/16042274/definition-of-whitespace-in-json
    if( ( c == ' ' || c == '\t' || c == '\n' || c == '\r' ) &&
        !( m_state == STATE_IN_STRING || m_state == STATE_UNICODE || m_state == STATE_START_ESCAPE ||
           m_state == STATE_IN_NUMBER ) ) {
        return;
    }
    switch( m_state ) {
    case STATE_IN_STRING:
        if( c == '"' ) {
            endString();
        }
        else if( c == '\\' ) {
            m_state = STATE_START_ESCAPE;
        }
        else if( ( c < 0x1f ) || ( c == 0x7f ) ) {
            // throw new RuntimeException("Unescaped control character encountered: " + c + " at position" +
            // characterCounter);
            onError( Error::Unescaped_control_character );
        }
        else {
            m_buffer[m_bufferPos] = c;
            increaseBufferPointer();
        }
        break;
    case STATE_IN_ARRAY:
        if( c == ']' ) {
            endArray();
        }
        else {
            startValue( c );
        }
        break;
    case STATE_IN_OBJECT:
        if( c == '}' ) {
            endObject();
        }
        else if( c == '"' ) {
            startKey();
        }
        else {
            // throw new RuntimeException("Start of string expected for object key. Instead got: " + c + " at position"
            // + characterCounter);
            onError( Error::Start_of_string_expected );
        }
        break;
    case STATE_END_KEY:
        if( c != ':' ) {
            // throw new RuntimeException("Expected ':' after key. Instead got " + c + " at position" +
            // characterCounter);
            onError( Error::Expected_double_point_after_key );
        }
        m_state = STATE_AFTER_KEY;
        break;
    case STATE_AFTER_KEY:
        startValue( c );
        break;
    case STATE_START_ESCAPE:
        processEscapeCharacters( c );
        break;
    case STATE_UNICODE:
        processUnicodeCharacter( c );
        break;
    case STATE_UNICODE_SURROGATE:
        m_unicodeEscapeBuffer[m_unicodeEscapeBufferPos] = c;
        m_unicodeEscapeBufferPos++;
        if( m_unicodeEscapeBufferPos == 2 ) {
            endUnicodeSurrogateInterstitial();
        }
        break;
    case STATE_AFTER_VALUE: {
        // not safe for size == 0!!!
        int within = m_stack[m_stackPos - 1];
        if( within == STACK_OBJECT ) {
            if( c == '}' ) {
                endObject();
            }
            else if( c == ',' ) {
                m_state = STATE_IN_OBJECT;
            }
            else {
                // throw new RuntimeException("Expected ',' or '}' while parsing object. Got: " + c + ". " +
                // characterCounter);
                onError( Error::Expected_comma_while_parsing_object );
            }
        }
        else if( within == STACK_ARRAY ) {
            if( c == ']' ) {
                endArray();
            }
            else if( c == ',' ) {
                m_state = STATE_IN_ARRAY;
            }
            else {
                // throw new RuntimeException("Expected ',' or ']' while parsing array. Got: " + c + ". " +
                // characterCounter);
            }
        }
        else {
            // throw new RuntimeException("Finished a literal, but unclear what state to move to. Last state: " +
            // characterCounter);
            onError( Error::Unclear_what_state_to_move_to );
        }
    } break;
    case STATE_IN_NUMBER:
        if( c >= '0' && c <= '9' ) {
            m_buffer[m_bufferPos] = c;
            increaseBufferPointer();
        }
        else if( c == '.' ) {
            if( doesCharArrayContain( m_buffer, m_bufferPos, '.' ) ) {
                // throw new RuntimeException("Cannot have multiple decimal points in a number. " + characterCounter);
                onError( Error::Cannot_have_multiple_decimal_points_in_a_number );
            }
            else if( doesCharArrayContain( m_buffer, m_bufferPos, 'e' ) ) {
                // throw new RuntimeException("Cannot have a decimal point in an exponent." + characterCounter);
                onError( Error::Cannot_have_a_decimal_point_in_an_exponent );
            }
            m_buffer[m_bufferPos] = c;
            increaseBufferPointer();
        }
        else if( c == 'e' || c == 'E' ) {
            if( doesCharArrayContain( m_buffer, m_bufferPos, 'e' ) ) {
                // throw new RuntimeException("Cannot have multiple exponents in a number. " + characterCounter);
                onError( Error::Cannot_have_multiple_exponents_in_a_number );
            }
            m_buffer[m_bufferPos] = c;
            increaseBufferPointer();
        }
        else if( c == '+' || c == '-' ) {
            char last = m_buffer[m_bufferPos - 1];
            if( !( last == 'e' || last == 'E' ) ) {
                // throw new RuntimeException("Can only have '+' or '-' after the 'e' or 'E' in a number." +
                // characterCounter);
                onError( Error::Can_only_have_plus_or_minus_after_the_e_or_E_in_a_number );
            }
            m_buffer[m_bufferPos] = c;
            increaseBufferPointer();
        }
        else {
            endNumber();
            // we have consumed one beyond the end of the number
            parse( c );
        }
        break;
    case STATE_IN_TRUE:
        m_buffer[m_bufferPos] = c;
        increaseBufferPointer();
        if( m_bufferPos == 4 ) {
            endTrue();
        }
        break;
    case STATE_IN_FALSE:
        m_buffer[m_bufferPos] = c;
        increaseBufferPointer();
        if( m_bufferPos == 5 ) {
            endFalse();
        }
        break;
    case STATE_IN_NULL:
        m_buffer[m_bufferPos] = c;
        increaseBufferPointer();
        if( m_bufferPos == 4 ) {
            endNull();
        }
        break;
    case STATE_START_DOCUMENT:
        m_myListener->startDocument();
        if( c == '[' ) {
            startArray();
        }
        else if( c == '{' ) {
            startObject();
        }
        else {
            // throw new ParsingError($this->_line_number,
            // $this->_char_number,
            // "Document must start with object or array.");
            onError( Error::Document_must_start_with_object_or_array );
        }
        break;
    case STATE_DONE:
        // throw new ParsingError($this->_line_number, $this->_char_number,
        // "Expected end of document.");
        onError( Error::Expected_end_of_document );
        break;
    default:
        // throw new ParsingError($this->_line_number, $this->_char_number,
        // "Internal error. Reached an unknown state: ".$this->_state);
        onError( Error::Reached_an_unknown_state );
    }
}

void JsonStreamingParser::increaseBufferPointer() {
    m_bufferPos = std::min( m_bufferPos + 1, BUFFER_MAX_LENGTH - 1 );
}

void JsonStreamingParser::endString() {
    int popped = m_stack[m_stackPos - 1];
    m_stackPos--;
    if( popped == STACK_KEY ) {
        m_buffer[m_bufferPos] = '\0';
        m_keysStack.push( std::make_tuple( depth(), StringType( m_buffer ) ) );
        m_myListener->key( currKey() );
        m_state = STATE_END_KEY;
    }
    else if( popped == STACK_STRING ) {
        m_buffer[m_bufferPos] = '\0';
        m_myListener->value( StringType( m_buffer ) );
        m_state = STATE_AFTER_VALUE;
        tryPopKey();
    }
    else {
        // throw new ParsingError($this->_line_number, $this->_char_number,
        // "Unexpected end of string.");
        onError( Error::Unexpected_end_of_string );
    }
    m_bufferPos = 0;
}

void JsonStreamingParser::startValue( char c ) {
    if( c == '[' ) {
        startArray();
    }
    else if( c == '{' ) {
        startObject();
    }
    else if( c == '"' ) {
        startString();
    }
    else if( isDigit( c ) ) {
        startNumber( c );
    }
    else if( c == 't' ) {
        m_state = STATE_IN_TRUE;
        m_buffer[m_bufferPos] = c;
        increaseBufferPointer();
    }
    else if( c == 'f' ) {
        m_state = STATE_IN_FALSE;
        m_buffer[m_bufferPos] = c;
        increaseBufferPointer();
    }
    else if( c == 'n' ) {
        m_state = STATE_IN_NULL;
        m_buffer[m_bufferPos] = c;
        increaseBufferPointer();
    }
    else {
        // throw new ParsingError($this->_line_number, $this->_char_number,
        // "Unexpected character for value: ".$c);
        onError( Error::Unexpected_character_for_value );
    }
}

bool JsonStreamingParser::isDigit( char c ) {
    // Only concerned with the first character in a number.
    return ( c >= '0' && c <= '9' ) || c == '-';
}

void JsonStreamingParser::endArray() {
    int popped = m_stack[m_stackPos - 1];
    m_stackPos--;
    if( popped != STACK_ARRAY ) {
        // throw new ParsingError($this->_line_number, $this->_char_number,
        // "Unexpected end of array encountered.");
        onError( Error::Unexpected_end_of_array_encountered );
    }
    m_myListener->endArray();
    m_state = STATE_AFTER_VALUE;
    tryPopKey();
    --m_depth;
    if( m_stackPos == 0 ) {
        endDocument();
    }
}

void JsonStreamingParser::startKey() {
    m_stack[m_stackPos] = STACK_KEY;
    m_stackPos++;
    m_state = STATE_IN_STRING;
}

void JsonStreamingParser::endObject() {
    int popped = m_stack[m_stackPos - 1];
    m_stackPos--;
    if( popped != STACK_OBJECT ) {
        // throw new ParsingError($this->_line_number, $this->_char_number,
        // "Unexpected end of object encountered.");
        onError( Error::Unexpected_end_of_object_encountered );
    }
    m_myListener->endObject();
    m_state = STATE_AFTER_VALUE;
    tryPopKey();
    --m_depth;
    if( m_stackPos == 0 ) {
        endDocument();
    }
}

void JsonStreamingParser::onError( Error error ) {
    m_myListener->error( int( error ) );
}

void JsonStreamingParser::tryPopKey() {
    if( m_stackPos == 0 ) {
        return;
    }

    if( m_keysStack.empty() ) {
        FUNC_LOGW( "Try operate on empty keys stack!" );
        return;
    }

    if( depth() == std::get<0>( m_keysStack.top() ) ) {
        m_keysStack.pop();
    }
}

void JsonStreamingParser::processEscapeCharacters( char c ) {
    if( c == '"' ) {
        m_buffer[m_bufferPos] = '"';
        increaseBufferPointer();
    }
    else if( c == '\\' ) {
        m_buffer[m_bufferPos] = '\\';
        increaseBufferPointer();
    }
    else if( c == '/' ) {
        m_buffer[m_bufferPos] = '/';
        increaseBufferPointer();
    }
    else if( c == 'b' ) {
        m_buffer[m_bufferPos] = 0x08;
        increaseBufferPointer();
    }
    else if( c == 'f' ) {
        m_buffer[m_bufferPos] = '\f';
        increaseBufferPointer();
    }
    else if( c == 'n' ) {
        m_buffer[m_bufferPos] = '\n';
        increaseBufferPointer();
    }
    else if( c == 'r' ) {
        m_buffer[m_bufferPos] = '\r';
        increaseBufferPointer();
    }
    else if( c == 't' ) {
        m_buffer[m_bufferPos] = '\t';
        increaseBufferPointer();
    }
    else if( c == 'u' ) {
        m_state = STATE_UNICODE;
    }
    else {
        // throw new ParsingError($this->_line_number, $this->_char_number,
        // "Expected escaped character after backslash. Got: ".$c);
        onError( Error::Expected_escaped_character_after_backslash );
    }
    if( m_state != STATE_UNICODE ) {
        m_state = STATE_IN_STRING;
    }
}

void JsonStreamingParser::processUnicodeCharacter( char c ) {
    if( !isHexCharacter( c ) ) {
        // throw new ParsingError($this->_line_number, $this->_char_number,
        // "Expected hex character for escaped Unicode character. Unicode parsed: "
        // . implode($this->_unicode_buffer) . " and got: ".$c);
        onError( Error::Expected_hex_character_for_escaped_Unicode_character );
    }

    m_unicodeBuffer[m_unicodeBufferPos] = c;
    m_unicodeBufferPos++;

    if( m_unicodeBufferPos == 4 ) {
        int codepoint = getHexArrayAsDecimal( m_unicodeBuffer, m_unicodeBufferPos );
        endUnicodeCharacter( codepoint );
        return;
        /*if (codepoint >= 0xD800 && codepoint < 0xDC00) {
          unicodeHighSurrogate = codepoint;
          unicodeBufferPos = 0;
          state = STATE_UNICODE_SURROGATE;
        } else if (codepoint >= 0xDC00 && codepoint <= 0xDFFF) {
          if (unicodeHighSurrogate == -1) {
            // throw new ParsingError($this->_line_number,
            // $this->_char_number,
            // "Missing high surrogate for Unicode low surrogate.");
            onError(Error::Missing_high_surrogate_for_Unicode_low_surrogate);
          }
          int combinedCodePoint = ((unicodeHighSurrogate - 0xD800) * 0x400) + (codepoint - 0xDC00) + 0x10000;
          endUnicodeCharacter(combinedCodePoint);
        } else if (unicodeHighSurrogate != -1) {
          // throw new ParsingError($this->_line_number,
          // $this->_char_number,
          // "Invalid low surrogate following Unicode high surrogate.");
          onError(Error::Invalid_low_surrogate_following_Unicode_high_surrogate);
          endUnicodeCharacter(codepoint);
        } else {
          endUnicodeCharacter(codepoint);
        }*/
    }
}

bool JsonStreamingParser::isHexCharacter( char c ) {
    return ( c >= '0' && c <= '9' ) || ( c >= 'a' && c <= 'f' ) || ( c >= 'A' && c <= 'F' );
}

int JsonStreamingParser::getHexArrayAsDecimal( char hexArray[], int length ) {
    int result = 0;
    for( int i = 0; i < length; i++ ) {
        char current = hexArray[length - i - 1];
        int value = 0;
        if( current >= 'a' && current <= 'f' ) {
            value = current - 'a' + 10;
        }
        else if( current >= 'A' && current <= 'F' ) {
            value = current - 'A' + 10;
        }
        else if( current >= '0' && current <= '9' ) {
            value = current - '0';
        }
        result += value * 16 ^ i;
    }
    return result;
}

bool JsonStreamingParser::doesCharArrayContain( char myArray[], int length, char c ) {
    for( int i = 0; i < length; i++ ) {
        if( myArray[i] == c ) {
            return true;
        }
    }
    return false;
}

void JsonStreamingParser::endUnicodeSurrogateInterstitial() {
    char unicodeEscape = m_unicodeEscapeBuffer[m_unicodeEscapeBufferPos - 1];
    if( unicodeEscape != 'u' ) {
        // throw new ParsingError($this->_line_number, $this->_char_number,
        // "Expected '\\u' following a Unicode high surrogate. Got: " .
        // $unicode_escape);
        onError( Error::Expected_u_following_a_Unicode_high_surrogate );
    }
    m_unicodeBufferPos = 0;
    m_unicodeEscapeBufferPos = 0;
    m_state = STATE_UNICODE;
}

void JsonStreamingParser::endNumber() {
    m_buffer[m_bufferPos] = '\0';
    StringType value = StringType( m_buffer );
    {
        const char* cstrBegin = value.c_str();
        char* cstrEnd;
        int ival = strtol( cstrBegin, &cstrEnd, 10 );
        typename String::size_type sz = cstrEnd - cstrBegin;
        if( sz == value.length() ) {
            m_myListener->value( ival );
        }
        else {
            float fval = strtof( cstrBegin, &cstrEnd );
            m_myListener->value( fval );
        }
        tryPopKey();
    }

    m_bufferPos = 0;
    m_state = STATE_AFTER_VALUE;
}

int JsonStreamingParser::convertDecimalBufferToInt( char myArray[], int length ) {
    int result = 0;
    for( int i = 0; i < length; i++ ) {
        char current = myArray[length - i - 1];
        result += ( current - '0' ) * 10;
    }
    return result;
}

void JsonStreamingParser::endDocument() {
    m_state = STATE_DONE;
    m_myListener->endDocument();
}

void JsonStreamingParser::endTrue() {
    m_buffer[m_bufferPos] = '\0';
    StringType value = StringType( m_buffer );
    if( value == "true" ) {
        m_myListener->value( true );
        tryPopKey();
    }
    else {
        // throw new ParsingError($this->_line_number, $this->_char_number,
        // "Expected 'true'. Got: ".$true);
        onError( Error::Expected_true );
    }
    m_bufferPos = 0;
    m_state = STATE_AFTER_VALUE;
}

void JsonStreamingParser::endFalse() {
    m_buffer[m_bufferPos] = '\0';
    StringType value = StringType( m_buffer );
    if( value == "false" ) {
        m_myListener->value( false );
        tryPopKey();
    }
    else {
        // throw new ParsingError($this->_line_number, $this->_char_number,
        // "Expected 'true'. Got: ".$true);
        onError( Error::Expected_false );
    }
    m_bufferPos = 0;
    m_state = STATE_AFTER_VALUE;
}

void JsonStreamingParser::endNull() {
    m_buffer[m_bufferPos] = '\0';
    StringType value = StringType( m_buffer );
    if( value == "null" ) {
        m_myListener->value( nullptr );
        tryPopKey();
    }
    else {
        // throw new ParsingError($this->_line_number, $this->_char_number,
        // "Expected 'true'. Got: ".$true);
        onError( Error::Expected_nullptr );
    }
    m_bufferPos = 0;
    m_state = STATE_AFTER_VALUE;
}

void JsonStreamingParser::startArray() {
    ++m_depth;
    m_state = STATE_IN_ARRAY;
    m_stack[m_stackPos] = STACK_ARRAY;
    m_stackPos++;
    m_myListener->startArray();
}

void JsonStreamingParser::startObject() {
    ++m_depth;
    m_state = STATE_IN_OBJECT;
    m_stack[m_stackPos] = STACK_OBJECT;
    m_stackPos++;
    m_myListener->startObject();
}

void JsonStreamingParser::startString() {
    m_stack[m_stackPos] = STACK_STRING;
    m_stackPos++;
    m_state = STATE_IN_STRING;
}

void JsonStreamingParser::startNumber( char c ) {
    m_state = STATE_IN_NUMBER;
    m_buffer[m_bufferPos] = c;
    increaseBufferPointer();
}

void JsonStreamingParser::endUnicodeCharacter( int codepoint ) {
    m_buffer[m_bufferPos] = convertCodepointToCharacter( codepoint );
    increaseBufferPointer();
    m_unicodeBufferPos = 0;
    m_unicodeHighSurrogate = -1;
    m_state = STATE_IN_STRING;
}

char JsonStreamingParser::convertCodepointToCharacter( int num ) {
    if( num <= 0x7F )
        return (char)( num );
    // if(num<=0x7FF) return (char)((num>>6)+192) + (char)((num&63)+128);
    // if(num<=0xFFFF) return
    // chr((num>>12)+224).chr(((num>>6)&63)+128).chr((num&63)+128);
    // if(num<=0x1FFFFF) return
    // chr((num>>18)+240).chr(((num>>12)&63)+128).chr(((num>>6)&63)+128).chr((num&63)+128);
    return ' ';
}

}  // namespace jssp
