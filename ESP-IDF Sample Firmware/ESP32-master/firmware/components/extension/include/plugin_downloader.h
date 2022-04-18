#pragma once

#include <string>
#include <HTTPClient.h>
#include <TCPSocket.h>

namespace plugin {

namespace impl {

class PluginDownloaderImpl {
public:
    PluginDownloaderImpl( const std::string& sourceURI, const std::string& filename );
    PluginDownloaderImpl( const PluginDownloaderImpl& other ) = delete;
    PluginDownloaderImpl( PluginDownloaderImpl&& other ) = delete;
    PluginDownloaderImpl& operator=( const PluginDownloaderImpl& other ) = delete;
    PluginDownloaderImpl& operator=( PluginDownloaderImpl&& other ) = delete;
    bool execute();

private:
    std::string m_sourceURI;
    std::string m_filename;
    network::TCPSocket m_tcpSock;
    network::HTTPClient m_httpClient;
};

}  // namespace impl

class PluginDownloader {
public:
    static constexpr uint32_t TASK_STACK_SIZE = 4096;
    PluginDownloader() = default;
    ~PluginDownloader() = default;
    PluginDownloader( const PluginDownloader& other ) = delete;
    PluginDownloader( PluginDownloader&& other ) = delete;
    PluginDownloader& operator=( const PluginDownloader& other ) = delete;
    PluginDownloader& operator=( PluginDownloader&& other ) = delete;
    bool download( const std::string& sourceURI, const std::string& filename );
    bool asyncDownload( const std::string& sourceURI, const std::string& filename );

private:
    static void task( void* pvParameter );
};

}  // namespace plugin
