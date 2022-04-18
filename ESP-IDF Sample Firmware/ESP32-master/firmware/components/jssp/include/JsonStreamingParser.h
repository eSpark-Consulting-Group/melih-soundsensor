
#pragma once

#include "JsonListener.h"

#include <string>
#include <tuple>
#include <stack>

namespace common {
class IReadStream;
}  // namespace common

namespace jssp {

enum : int {
    STATE_START_DOCUMENT = 0,
    STATE_DONE = -1,
    STATE_IN_ARRAY = 1,
    STATE_IN_OBJECT = 2,
    STATE_END_KEY = 3,
    STATE_AFTER_KEY = 4,
    STATE_IN_STRING = 5,
    STATE_START_ESCAPE = 6,
    STATE_UNICODE = 7,
    STATE_IN_NUMBER = 8,
    STATE_IN_TRUE = 9,
    STATE_IN_FALSE = 10,
    STATE_IN_NULL = 11,
    STATE_AFTER_VALUE = 12,
    STATE_UNICODE_SURROGATE = 13,

    STACK_OBJECT = 0,
    STACK_ARRAY = 1,
    STACK_KEY = 2,
    STACK_STRING = 3,

    BUFFER_MAX_LENGTH = 512,
    STACK_MAX_SIZE = 20,
    UNICODE_BUFFER_SIZE = 10,
};

enum class Error : int {
    Unescaped_control_character = -1,
    Start_of_string_expected = -2,
    Expected_double_point_after_key = -3,
    Expected_comma_while_parsing_object = -4,
    Unclear_what_state_to_move_to = -5,
    Cannot_have_multiple_decimal_points_in_a_number = -6,
    Cannot_have_a_decimal_point_in_an_exponent = -7,
    Cannot_have_multiple_exponents_in_a_number = -8,
    Can_only_have_plus_or_minus_after_the_e_or_E_in_a_number = -9,
    Document_must_start_with_object_or_array = -10,
    Expected_end_of_document = -11,
    Reached_an_unknown_state = -12,
    Unexpected_end_of_string = -13,
    Unexpected_character_for_value = -14,
    Unexpected_end_of_array_encountered = -15,
    Unexpected_end_of_object_encountered = -16,
    Expected_escaped_character_after_backslash = -17,
    Expected_hex_character_for_escaped_Unicode_character = -18,
    Missing_high_surrogate_for_Unicode_low_surrogate = -19,
    Invalid_low_surrogate_following_Unicode_high_surrogate = -20,
    Expected_u_following_a_Unicode_high_surrogate = -21,
    Expected_true = -22,
    Expected_false = -23,
    Expected_nullptr = -24,
};

using KeysStackType = std::stack<std::tuple<int, std::string>>;

struct ParserState {
    int m_state;
    int stack[STACK_MAX_SIZE];
    int stackPos;
    int depth;
    KeysStackType m_keysStack;
};

class JsonStreamingParser {
    using String = std::string;

    void increaseBufferPointer();

    void endString();

    void endArray();

    void startValue( char c );

    void startKey();

    void processEscapeCharacters( char c );

    bool isDigit( char c );

    bool isHexCharacter( char c );

    char convertCodepointToCharacter( int num );

    void endUnicodeCharacter( int codepoint );

    void startNumber( char c );

    void startString();

    void startObject();

    void startArray();

    void endNull();

    void endFalse();

    void endTrue();

    void endDocument();

    int convertDecimalBufferToInt( char myArray[], int length );

    void endNumber();

    void endUnicodeSurrogateInterstitial();

    bool doesCharArrayContain( char myArray[], int length, char c );

    int getHexArrayAsDecimal( char hexArray[], int length );

    void processUnicodeCharacter( char c );

    void endObject();

    void onError( Error error );

    void tryPopKey();

public:
    using StringType = String;
    using JsonListener = JsonListenerBase;

    JsonStreamingParser();

    JsonStreamingParser( JsonListener* listener )
    : m_myListener( listener ) {}

    virtual ~JsonStreamingParser() = default;

    void parse( char c );

    void setListener( JsonListener* listener );

    virtual bool reset();

    void storeState( ParserState& state ) const;

    void restoreState( const ParserState& state );

    inline int depth() const {
        return m_depth;
    }

    inline int state() const {
        return m_state;
    }

    inline auto currKey() const {
        return m_keysStack.empty() ? StringType() : std::get<1>( m_keysStack.top() );
    }

private:
    int m_state;
    int m_stack[STACK_MAX_SIZE];
    int m_stackPos = 0;
    int m_depth = 0;
    JsonListener* m_myListener;

    KeysStackType m_keysStack;

    // bool doEmitWhitespace = false;
    // fixed length buffer array to prepare for c code
    char m_buffer[BUFFER_MAX_LENGTH];
    int m_bufferPos = 0;

    char m_unicodeEscapeBuffer[UNICODE_BUFFER_SIZE];
    int m_unicodeEscapeBufferPos = 0;

    char m_unicodeBuffer[UNICODE_BUFFER_SIZE];
    int m_unicodeBufferPos = 0;

    int m_unicodeHighSurrogate = 0;
};

}  // namespace jssp
