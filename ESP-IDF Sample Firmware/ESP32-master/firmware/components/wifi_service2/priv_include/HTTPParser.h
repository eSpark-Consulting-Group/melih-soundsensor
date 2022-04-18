
#pragma once

#include <cinttypes>

namespace network {

class HTTPParser {
    enum class State { ParseHeader, ParseData, MessageFinished };

public:
    enum class HttpMethod { GET, POST, Unknown };

public:
    virtual ~HTTPParser() = default;

    bool parse( const char* receivedData, std::size_t len );

protected:
    virtual void processFinishedMessage( const char* buffer, std::size_t size ) = 0;

    virtual void processUrlHeader( const char* url, std::size_t size ) = 0;

    virtual char* getBuffer() = 0;

    virtual std::size_t getMaxBufferSize() = 0;

    HttpMethod getMethod();

private:
    void parseHttpHeader( const char currentSymbol );

    void parseHttpData( const char currentSymbol );

    void checkEndLines( const char currentSymbol );

    bool hasDoubleEnd() const;

    bool hasOneEnd() const;

    void resetEndMarkers();

    void resetToDefault();

private:
    HttpMethod m_method = HttpMethod::Unknown;
    State m_state = State::ParseHeader;

    bool m_hasREnd[2] = { false, false };
    bool m_hasNEnd[2] = { false, false };

    std::size_t m_expectedDataSize = 0;
    std::size_t m_dataSize = 0;
};

}  // namespace network
