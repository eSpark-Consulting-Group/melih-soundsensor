#pragma once

#include <esp_http_client.h>
#include <functional>
#include <map>
#include <functional>

namespace network {

class HTTPRequest {
public:
    HTTPRequest( const std::string& url, esp_http_client_method_t method = HTTP_METHOD_GET );

    HTTPRequest( const std::string& url, const std::string& method )
    : HTTPRequest( url, methodToEnum( method ) ) {}

    void setUrl( const std::string& url );
    void setContentType( const std::string& contentType );
    void setContent( const std::string& content );
    void setPostData( const std::vector<char>& postData );
    void setCredential( const std::string& login, const std::string& pass );
    void setHeaders( const std::map<std::string, std::string> headers );
    void setSkipSecurity( bool skip );
    void setDownloadPath( const std::string& path );

    void setReceiveHandler( std::function<void( int )> );
    void setConnectCloseHandler( std::function<void()> );
    void setConnectDownloadingProgressHandler( std::function<void( uint8_t progress )> );

    int send();
    bool uploadFile( const std::string filename, std::function<void( const char* answer )> );

private:
    static esp_err_t httpEventHandle( esp_http_client_event_t* evt );
    static esp_http_client_method_t methodToEnum( std::string method );

private:
    std::string m_url;
    esp_http_client_method_t m_requestType;
    std::string m_contentType;
    std::string m_content;
    std::vector<char> m_postData;
    std::string m_login;
    std::string m_pass;
    std::map<std::string, std::string> m_headers;
    bool m_skipSecurity = true;
    std::string m_path;
    std::function<void( int status )> m_receiveHandler = nullptr;
    std::function<void()> m_connectCloseHandler = nullptr;
    std::function<void( int progress )> m_connectProgressHandler = nullptr;
};

}  // namespace network
