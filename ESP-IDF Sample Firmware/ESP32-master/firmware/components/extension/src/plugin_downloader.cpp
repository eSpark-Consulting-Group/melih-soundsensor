#include <plugin_downloader.h>

#include <string>
#include <chrono>
#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <DebugUtils.h>

#include <Storage.h>
#include <StorageFile.h>
#include <HTTPData/HTTPFile.h>
#include <soc/soc.h>

namespace plugin {

namespace impl {

PluginDownloaderImpl::PluginDownloaderImpl( const std::string& sourceURI, const std::string& filename )
: m_sourceURI( sourceURI )
, m_filename( filename )
, m_httpClient( m_tcpSock ) {}

bool PluginDownloaderImpl::execute() {
    if( storage::Storage::get().isExist( m_filename.c_str() ) ) {
        if( !storage::Storage::get().remove( m_filename.c_str() ) ) {
            FUNC_LOGE( "Failed to remove old file" );
            return false;
        }
    }
    network::HTTPFile plugin_writer( m_filename );
    if( network::HTTPResult::OK == m_httpClient.get( m_sourceURI.c_str(), &plugin_writer ) ) {
        FUNC_LOGI( "Plugin downloaded to: %s. File size: %d", m_filename.c_str(), plugin_writer.getWrittenByteCount() );
    }
    else {
        FUNC_LOGE( "Failed downloading plugin from server" );
        storage::Storage::get().remove( m_filename.c_str() );
        return false;
    }
    return true;
}

}  // namespace impl

bool PluginDownloader::download( const std::string& sourceURI, const std::string& filename ) {
    if( sourceURI.empty() || filename.empty() ) {
        return false;
    }
    impl::PluginDownloaderImpl installer( sourceURI, filename );
    return installer.execute();
}

namespace {
struct TaskParameter {
    std::unique_ptr<impl::PluginDownloaderImpl> downloader;
    // TODO:: Add parameter for async notification
};
}  // namespace

bool PluginDownloader::asyncDownload( const std::string& sourceURI, const std::string& filename ) {
    if( sourceURI.empty() || filename.empty() ) {
        return false;
    }
    TaskParameter* params = new TaskParameter;
    params->downloader = std::make_unique<impl::PluginDownloaderImpl>( sourceURI, filename );
    TaskHandle_t handleOfTask;
    if( pdPASS != xTaskCreate( &PluginDownloader::task,
                               "PLUGIN_DOWNLOAD",
                               TASK_STACK_SIZE,
                               reinterpret_cast<void*>( params ),
                               tskIDLE_PRIORITY + 1,
                               &handleOfTask ) ) {
        delete params;
        return false;
    };
    return true;
}

void PluginDownloader::task( void* pvParameter ) {
    TaskParameter* params = reinterpret_cast<TaskParameter*>( pvParameter );
    params->downloader->execute();
    // TODO: Send notification

    delete params;
    vTaskDelete( NULL );
};

}  // namespace plugin
