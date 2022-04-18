
#pragma once

#include <cinttypes>

namespace common {

class IValue;

class JsonStreamParser {
    struct Flags {
        enum Enum : std::uint8_t {

            Empty = 0,
            NotQuotedToken = 1 << 0,
            Value = 1 << 1,
            QuotedToken = 1 << 2,
            FloatToken = 1 << 3,
            TokenTruncated = 1 << 4
        };
    };

public:
    JsonStreamParser( char* buffer, std::size_t size );

    virtual ~JsonStreamParser() = default;

    bool parse( const char* buf, std::size_t len );

    void ignoreAllNestedBlocks();

    bool isQuotedTokenProcessing() const;

    bool isValueProcessing() const;

    bool isNotQuotedTokenProcessing() const;

    bool isFloatTokenProcessing() const;

    const char* getTokenValue() const;

    std::size_t getTokenSize() const;

    bool isSame( const char* token, std::size_t tokenSize ) const;

    bool isSame( const char* token ) const;

    bool isSameInOneCase( const char* token, std::size_t tokenSize ) const;

    bool isSameInOneCase( const char* token ) const;

    unsigned int getCurrentDepth() const;

    void resetTokenProcessing();

public:
    bool read( bool& value ) const;

    bool read( float& value ) const;

    bool read( int& value ) const;

    bool read( uint16_t& value ) const;

    bool read( IValue*& value ) const;

    bool readHex( uint32_t& value ) const;

protected:
    virtual bool processActiveToken() = 0;

    virtual bool onStepIn();

    virtual bool onStepOut();

    virtual void onSymbolParsed( char symbol );

    /**
     * @brief Called when token doesn't fit into buffer
     *
     * @return true Internal buffer will be cleared and filled with the following symbols
     * @return false Internal buffer will hold trancated value and the rest symbols will be discarded
     */
    virtual bool onLongToken();

private:
    bool isActiveBlockNeededToParse() const;

    void setTokenTruncated();

    void enableQuotedTokenProcessing();

    void enableFloatTokenProcessing();

    void enableNotQuotedTokenProcessing();

    void enableValueTokenProcessing();

    void disableValueTokenProcessing();

    bool processActiveTokenAndResetState();

    void addCharToToken( char value );

    void goHigher();

    void goDeeper();

private:
    std::uint8_t m_stateFlags = Flags::Empty;

    std::size_t m_depth = 0;
    std::size_t m_ignoreLevel = 0;

    std::size_t m_activeTokenSize = 0;
    std::size_t m_maxBufferSize = 0;
    char* m_buffer = nullptr;
};

}  // namespace common
