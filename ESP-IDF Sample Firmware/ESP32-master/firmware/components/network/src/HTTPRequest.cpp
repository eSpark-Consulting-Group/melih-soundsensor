
#include "HTTPRequest.h"
#include "DebugUtils.h"

#include <esp_system.h>
#include <esp_http_client.h>

#include <memory.h>
#include <fstream>

namespace network {

constexpr auto MaxHttpRecvBuffer = 1024;
constexpr auto MaxHttpOutputBuffer = 1024;
constexpr auto RequestTimeout = 8000;
constexpr auto FileReadBufferSize = 512;
constexpr auto MaxAttemptsNumber = 500;

esp_err_t HTTPRequest::httpEventHandle( esp_http_client_event_t* evt ) {
    static int output_len = 0;  // Stores number of bytes read
    static std::fstream file;
    static int contentLength = 0;

    HTTPRequest* request = (HTTPRequest*)evt->user_data;

    switch( evt->event_id ) {
    case HTTP_EVENT_ERROR:
        FUNC_LOGI( "HTTP_EVENT_ERROR" );
        output_len = 0;
        contentLength = 0;
        break;
    case HTTP_EVENT_ON_CONNECTED:
        FUNC_LOGI( "HTTP_EVENT_ON_CONNECTED" );
        if( request->m_receiveHandler ) {
            request->m_receiveHandler( 200 );
        }

        output_len = 0;
        contentLength = 0;
        break;
    case HTTP_EVENT_HEADER_SENT:
        FUNC_LOGI( "HTTP_EVENT_HEADER_SENT" );
        break;
    case HTTP_EVENT_ON_HEADER:
        FUNC_LOGI( "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value );
        break;
    case HTTP_EVENT_ON_DATA:
        FUNC_LOGI( "HTTP_EVENT_ON_DATA: len: %d", evt->data_len );
        if( !output_len ) {
            if( request && !request->m_path.empty() ) {
                file.open( request->m_path, std::ios::out | std::ios_base::binary );
                contentLength = esp_http_client_get_content_length( evt->client );
            }
        }

        if( !esp_http_client_is_chunked_response( evt->client ) ) {
            if( request && request->m_receiveHandler ) {
                request->m_connectProgressHandler( 100 * output_len / contentLength );
            }
        }

        output_len += evt->data_len;
        if( evt->data_len > 0 ) {
            file.write( (char*)evt->data, evt->data_len );
        }

        break;
    case HTTP_EVENT_ON_FINISH:
        FUNC_LOGI( "HTTP_EVENT_ON_FINISH" );
        if( output_len == contentLength ) {
            file.close();
            if( request && request->m_receiveHandler ) {
                request->m_connectProgressHandler( 100 );
            }
        }
        output_len = 0;
        contentLength = 0;
        break;
    case HTTP_EVENT_DISCONNECTED:
        FUNC_LOGI( "HTTP_EVENT_DISCONNECTED" );
        if( request && request->m_connectCloseHandler ) {
            request->m_connectCloseHandler();
        }

        break;
    }
    return ESP_OK;
}

HTTPRequest::HTTPRequest( const std::string& url, esp_http_client_method_t method )
: m_url( url )
, m_requestType( method ) {}

void HTTPRequest::setUrl( const std::string& url ) {
    m_url = url;
}

void HTTPRequest::setContentType( const std::string& contentType ) {
    m_contentType = contentType;
}

void HTTPRequest::setDownloadPath( const std::string& path ) {
    m_path = path;
}

void HTTPRequest::setContent( const std::string& content ) {
    m_content = content;
}

void HTTPRequest::setPostData( const std::vector<char>& postData ) {
    m_postData = postData;
}

void HTTPRequest::setCredential( const std::string& login, const std::string& pass ) {
    m_login = login;
    m_pass = pass;
}

void HTTPRequest::setHeaders( const std::map<std::string, std::string> headers ) {
    m_headers = headers;
}

void HTTPRequest::setSkipSecurity( bool skip ) {
    m_skipSecurity = skip;
}

esp_http_client_method_t HTTPRequest::methodToEnum( std::string method ) {
    for( auto& ch : method ) {
        ch = toupper( ch );
    }

    if( method == "GET" ) {
        return HTTP_METHOD_GET;
    }
    else if( method == "POST" ) {
        return HTTP_METHOD_POST;
    }
    else if( method == "PUT" ) {
        return HTTP_METHOD_PUT;
    }
    else if( method == "DELETE" ) {
        return HTTP_METHOD_DELETE;
    }
    else {
        FUNC_LOGW( "Unknow method: %s. Use GET", method.c_str() );
        return HTTP_METHOD_GET;
    }
}

void HTTPRequest::setReceiveHandler( std::function<void( int )> func ) {
    m_receiveHandler = func;
}

void HTTPRequest::setConnectCloseHandler( std::function<void()> func ) {
    m_connectCloseHandler = func;
}

void HTTPRequest::setConnectDownloadingProgressHandler( std::function<void( uint8_t progress )> func ) {
    m_connectProgressHandler = func;
}

bool HTTPRequest::uploadFile( const std::string filename, std::function<void( const char* answer )> clbk ) {
    FUNC_LOGI( "uploadFile %s", filename.c_str() );
    auto url = m_url;

    if( m_url.find( "http" ) == 0 ) {
        std::size_t found = m_url.find_first_of( "://" );
        url = m_url.substr( found + 3 );
    }

    char local_response_buffer[MaxHttpOutputBuffer] = { 0 };

    std::string host = url;
    std::string path = "/";

    const auto path_pos = url.find_first_of( path );
    if( path_pos != std::string::npos ) {
        host = url.substr( 0, path_pos );
        path = url.substr( path_pos );
    }

    FUNC_LOGI( "host: %s", host.c_str() );
    FUNC_LOGI( "path: %s", path.c_str() );

    esp_http_client_transport_t transport_type = HTTP_TRANSPORT_UNKNOWN;

    bool is_async = false;
    if( m_url.find( "https" ) == 0 ) {
        transport_type = HTTP_TRANSPORT_OVER_SSL;
        is_async = true;
    }

    esp_http_client_config_t config = { .host = host.c_str(),
                                        .path = path.c_str(),
                                        .method = m_requestType,
                                        .timeout_ms = RequestTimeout,
                                        .event_handler = httpEventHandle,
                                        .transport_type = transport_type,
                                        .buffer_size_tx = MaxHttpOutputBuffer,
                                        .user_data = (void*)this,
                                        .is_async = is_async,
                                        .skip_cert_common_name_check = false };

    esp_http_client_handle_t client = esp_http_client_init( &config );

    for( auto const& header : m_headers ) {
        esp_http_client_set_header( client, header.first.c_str(), header.second.c_str() );
    }

    char boundaryMarker[12] = { 0 };

    long long int timestamp;
    struct timeval tv;
    gettimeofday( &tv, NULL );
    timestamp = (long long int)tv.tv_sec * 1000 + tv.tv_usec;
    snprintf( boundaryMarker, 64, "%llx", timestamp );

    std::string baseFilename = filename.substr( filename.find_last_of( "/\\" ) + 1 );

    FUNC_LOGE( "baseFilename: %s", baseFilename.c_str() );

    std::string boundary = "--";
    boundary += boundaryMarker;
    boundary += "\r\n";

    std::string contentDisposition = "Content-Disposition: form-data; name=\"file\"; filename=\"";
    contentDisposition += baseFilename;
    contentDisposition += "\"\r\n";

    std::string contentType = "Content-Type: ";
    contentType += m_contentType;
    contentType += "\r\n\r\n";

    int fileLength = 0;
    std::ifstream is( filename, std::ifstream::binary );
    if( is ) {
        is.seekg( 0, is.end );
        fileLength = is.tellg();
        is.seekg( 0, is.beg );
    }

    std::string endBoundary = "\r\n--";
    endBoundary += boundaryMarker;
    endBoundary += "--\r\n";

    FUNC_LOGE( "boundary: %s", boundary.c_str() );
    FUNC_LOGE( "contentDisposition: %s", contentDisposition.c_str() );
    FUNC_LOGE( "contentType: %s", contentType.c_str() );
    FUNC_LOGE( "fileLength: %d", fileLength );
    FUNC_LOGE( "endBoundary: %s", endBoundary.c_str() );

    int contentLength =
        boundary.length() + contentDisposition.length() + contentType.length() + fileLength + endBoundary.length();

    esp_http_client_set_header( client, "Content-Length", std::to_string( contentLength ).c_str() );

    std::string contentTypeHeader = "multipart/form-data; ";
    contentTypeHeader += "boundary=";
    contentTypeHeader += boundaryMarker;

    FUNC_LOGE( "contentTypeHeader: %s", contentTypeHeader.c_str() );
    esp_http_client_set_header( client, "Content-Type", contentTypeHeader.c_str() );

    esp_err_t err;

    for( int attempt = 0; attempt < 1000; attempt++ ) {
        err = esp_http_client_open( client, contentLength );
        if( err == ESP_OK ) {
            break;
        }
    }

    if( err == ESP_OK ) {
        esp_http_client_write( client, boundary.c_str(), boundary.length() );
        esp_http_client_write( client, contentDisposition.c_str(), contentDisposition.length() );
        esp_http_client_write( client, contentType.c_str(), contentType.length() );

        if( is ) {
            for( int i = 0; i < fileLength / FileReadBufferSize + 1; i++ ) {
                char buffer[FileReadBufferSize];
                is.read( buffer, FileReadBufferSize );
                auto len = is.gcount();
                if( len > 0 ) {
                    esp_http_client_write( client, buffer, len );
                }
            }
        }

        esp_http_client_write( client, endBoundary.c_str(), endBoundary.length() );

        FUNC_LOGI( "fetch_headers:\t%d", esp_http_client_fetch_headers( client ) );
        FUNC_LOGI( "chunked:\t%d", esp_http_client_is_chunked_response( client ) );

        int responseLength = esp_http_client_get_content_length( client );
        FUNC_LOGI( "responseLength:\t%d", responseLength );

        int status = esp_http_client_get_status_code( client );
        FUNC_LOGI( "status:\t%d", status );

        if( responseLength ) {
            char buffer[MaxHttpRecvBuffer];
            bzero( buffer, MaxHttpRecvBuffer );

            if( esp_http_client_read( client, buffer, responseLength ) == responseLength )
                FUNC_LOGI( "Response: %.*s", responseLength, buffer );

            clbk( buffer );
        }

        err = esp_http_client_close( client );
        if( err != ESP_OK ) {
            esp_http_client_cleanup( client );
        }
    }
    else {
        FUNC_LOGE( "Error perform http request %s", esp_err_to_name( err ) );
        return false;
    }

    return true;
}

int HTTPRequest::send() {
    FUNC_LOGI( "HTTPRequest::send: %s", m_url.c_str() );

    std::string url = m_url;

    if( m_url.find( "http" ) == 0 ) {
        std::size_t found = m_url.find_first_of( "://" );
        url = m_url.substr( found + 3 );
    }

    char local_response_buffer[MaxHttpOutputBuffer] = { 0 };

    std::string host = url;
    std::string path = "/";

    const auto path_pos = url.find_first_of( path );
    if( path_pos != std::string::npos ) {
        host = url.substr( 0, path_pos );
        path = url.substr( path_pos );
    }

    esp_http_client_transport_t transport_type = HTTP_TRANSPORT_UNKNOWN;

    bool is_async = false;
    if( m_url.find( "https" ) == 0 ) {
        transport_type = HTTP_TRANSPORT_OVER_SSL;
        is_async = true;
    }

    esp_http_client_config_t config = { .host = host.c_str(),
                                        .path = path.c_str(),
                                        .method = m_requestType,
                                        .timeout_ms = RequestTimeout,
                                        .event_handler = httpEventHandle,
                                        .transport_type = transport_type,
                                        .buffer_size_tx = MaxHttpOutputBuffer,
                                        .user_data = (void*)this,
                                        .is_async = is_async,
                                        .skip_cert_common_name_check = !m_skipSecurity };

    esp_http_client_handle_t client = esp_http_client_init( &config );

    if( !m_login.empty() ) {
        esp_http_client_set_username( client, m_login.c_str() );
    }

    if( !m_pass.empty() ) {
        esp_http_client_set_password( client, (char*)m_pass.c_str() );
    }

    for( auto const& header : m_headers ) {
        esp_http_client_set_header( client, header.first.c_str(), header.second.c_str() );
    }

    if( !m_contentType.empty() ) {
        esp_http_client_set_header( client, "Content-Type", m_contentType.c_str() );
    }

    if( !m_content.empty() ) {
        esp_http_client_set_post_field( client, m_content.c_str(), m_content.length() );
    }

    if( !m_postData.empty() ) {
        esp_http_client_set_post_field( client, m_postData.data(), m_postData.size() );
    }

    esp_err_t err;

    for( int attempt = 0; attempt < MaxAttemptsNumber; attempt++ ) {
        err = esp_http_client_perform( client );
        if( err != ESP_ERR_HTTP_EAGAIN ) {
            break;
        }
    }
    if( err == ESP_OK ) {
        FUNC_LOGI( "HTTPS Status = %d, content_length = %d",
                   esp_http_client_get_status_code( client ),
                   esp_http_client_get_content_length( client ) );
    }
    else {
        FUNC_LOGE( "Error perform http request %d", err );
    }

    int res = esp_http_client_get_status_code( client );
    esp_http_client_cleanup( client );

    if( res == 0 ) {
        res = 404;
    }
    else if( res < 0 ) {
        res = 400;
    }

    FUNC_LOGI( "res: %d", res );
    return res;
}

}  // namespace network
