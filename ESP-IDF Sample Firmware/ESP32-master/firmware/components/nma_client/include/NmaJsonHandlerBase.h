#pragma once

#include "INmaJsonHandler.h"
#include "JsonStreamingParser.h"

#include "gason.hpp"
#include "JsonStreamGenerator.h"

#include <vector>
#include <memory>
#include <functional>

namespace nma {

class NmaJsonHandlerBase : public INmaJsonHandler {
public:
    using UPtr = std::unique_ptr<NmaJsonHandlerBase>;
    using StringType = std::string;
    using ResponceType = std::tuple<int, StringType>;
    using DataType = std::vector<char>;
    using JsonAllocatorUPtr = std::unique_ptr<gason::JsonAllocator>;

    class JsListener : public jssp::JsonListenerBase {
    public:
        JsListener( NmaJsonHandlerBase& handler )
        : m_handler( handler ) {}

        void startDocument() override{
            // Nothing to do
        };

        void endDocument() override {
            m_handler.onJsonDocumentReceived();
        };

        void error( int errorCode ) override {
            m_handler.onParseError( errorCode );
        }

    private:
        NmaJsonHandlerBase& m_handler;
    };

    class JsParser : public jssp::JsonStreamingParser {
    public:
        using jssp::JsonStreamingParser::JsonStreamingParser;
    };

    using TaskClbkType = std::function<ResponceType( const gason::JsonValue&, common::JsonStreamGenerator& )>;

    struct Context {
        Context( DataType&& data, common::IWriteStream& wstr )
        : jsData( std::move( data ) )
        , jsGen( wstr ) {}

        StringType requestId;
        DataType jsData;
        gason::JsonAllocator jsonAlloc;
        gason::JsonValue json;
        common::JsonStreamGenerator jsGen;
    };

#ifdef CONFIG_SPIRAM
    static constexpr auto DefaultBufSize = 1024;
#else   // not CONFIG_SPIRAM
    static constexpr auto DefaultBufSize = 256;
#endif  // CONFIG_SPIRAM

public:
    NmaJsonHandlerBase();

    bool parse( common::IReadStream& readStream, common::IWriteStream::UPtr writeStream ) override;

    void onJsonDocumentReceived();

    void onParseError( const int errorCode );

    void parseJsonDocument() override;

    void reset() override;

    bool hasDefferedTask() const override {
        return nullptr != m_taskClbk;
    }

    int updateDefferedTask() override;

private:
    void processMethod( const char* method, std::unique_ptr<Context>& context );

    ResponceType callCloudMethods( const char* method, const gason::JsonValue& json, common::JsonStreamGenerator& gen );

    ResponceType callBroadcastsMethods( const char* method,
                                        const gason::JsonValue& json,
                                        common::JsonStreamGenerator& gen );

    ResponceType callHubMethods( const char* method, const gason::JsonValue& json, common::JsonStreamGenerator& gen );

    void completeResponce( const StringType& requestId,
                           const gason::JsonValue& json,
                           common::JsonStreamGenerator& jsGen,
                           int errorCode,
                           StringType&& errorMessage );

protected:
    DataType m_data;

    JsListener m_jsListener;
    JsParser m_jsParser;
    bool m_isParsingFailed = false;
    common::IWriteStream::UPtr m_outStream;

    TaskClbkType m_taskClbk;
    std::unique_ptr<Context> m_context;
};

}  // namespace nma
