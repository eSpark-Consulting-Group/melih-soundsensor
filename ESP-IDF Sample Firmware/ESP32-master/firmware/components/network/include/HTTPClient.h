/* HTTPClient.h */
/* Copyright (C) 2012 mbed.org, MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <IHTTPData.h>

#include <cstdint>

namespace network {

constexpr int HttpClienDefaultTimeout = 5000;

class HTTPData;
class ISocket;

/// HTTP client results
enum class HTTPResult {
    OK,          ///< Success
    PROCESSING,  ///< Processing
    PARSE,       ///< url Parse error
    DNS,         ///< Could not resolve name
    PRTCL,       ///< Protocol error
    NOTFOUND,    ///< HTTP 404 Error
    REFUSED,     ///< HTTP 403 Error
    ERROR,       ///< HTTP xxx error
    TIMEOUT,     ///< Connection timeout
    CONN,        ///< Connection error
    CLOSED,      ///< Connection was closed by remote host

    Last = CLOSED
};

/**A simple HTTP Client
The HTTPClient is composed of:
- The actual client (HTTPClient)
- Classes that act as a data repository, each of which deriving from the HTTPData class (HTTPText for short text
content, HTTPFile for file I/O, HTTPMap for key/value pairs, and HTTPStream for streaming purposes)
*/
class HTTPClient {
public:
    /// Instantiate the HTTP client
    HTTPClient( ISocket& socket );
    ~HTTPClient();

    /// Get the text form of the error number
    ///
    /// Gets a pointer to a text message that described the result code.
    ///
    /// @param[in] res is the HTTPResult code to translate to text.
    /// @returns a pointer to a text message.
    ///
    static const char* GetErrorMessage( HTTPResult res );

    /**
    Provides a basic authentification feature (Base64 encoded username and password)
    Pass two NULL pointers to switch back to no authentication
    @param[in] user username to use for authentication, must remain valid durlng the whole HTTP session
    @param[in] user password to use for authentication, must remain valid durlng the whole HTTP session
    */
    void basicAuth( const char* user, const char* password );  // Basic Authentification

    void bearerAuth( const char* password );

    /**
    Set custom headers for request.
    Pass NULL, 0 to turn off custom headers.
    @code
     const char * hdrs[] =
            {
            "Connection", "keep-alive",
            "Accept", "text/html",
            "User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64)",
            "Accept-Encoding", "gzip,deflate,sdch",
            "Accept-Language", "en-US,en;q=0.8",
            };

        http.basicAuth("username", "password");
        http.customHeaders(hdrs, 5);
    @endcode

    @param[in] headers an array (size multiple of two) key-value pairs, must remain valid during the whole HTTP session
    @param[in] pairs number of key-value pairs
    */
    void customHeaders( const char** headers, size_t pairs );

    // High Level setup functions
    /** Execute a GET request on the URL
    Blocks until completion
    @param[in] url : url on which to execute the request
    @param[in,out] pDataIn : pointer to an IHTTPDataIn instance that will collect the data returned by the request, can
    be NULL
    @param[in] timeout waiting timeout in ms (osWaitForever for blocking function, not recommended)
    @return 0 on success, HTTP error (<0) on failure
    */
    HTTPResult get( const char* url, IHTTPDataIn* pDataIn, int timeout = HttpClienDefaultTimeout );  // Blocking

    /** Execute a GET request on the URL
    Blocks until completion
    This is a helper to directly get a piece of text from a HTTP result
    @param[in] url : url on which to execute the request
    @param[out] result : pointer to a char array in which the result will be stored
    @param[in] maxResultLen : length of the char array (including space for the NULL-terminating char)
    @param[in] timeout waiting timeout in ms (osWaitForever for blocking function, not recommended)
    @return 0 on success, HTTP error (<0) on failure
    */
    HTTPResult get( const char* url,
                    char* result,
                    size_t maxResultLen,
                    int timeout = HttpClienDefaultTimeout );  // Blocking

    /** Execute a POST request on the URL
    Blocks until completion
    @param[in] url : url on which to execute the request
    @param[out] dataOut : a IHTTPDataOut instance that contains the data that will be posted
    @param[in,out] pDataIn : pointer to an IHTTPDataIn instance that will collect the data returned by the request, can
    be NULL
    @param[in] timeout waiting timeout in ms (osWaitForever for blocking function, not recommended)
    @return 0 on success, HTTP error (<0) on failure
    */
    HTTPResult post( const char* url,
                     const IHTTPDataOut& dataOut,
                     IHTTPDataIn* pDataIn,
                     int timeout = HttpClienDefaultTimeout );  // Blocking

    /** Execute a PUT request on the URL
    Blocks until completion
    @param[in] url : url on which to execute the request
    @param[in] dataOut : a IHTTPDataOut instance that contains the data that will be put
    @param[in,out] pDataIn : pointer to an IHTTPDataIn instance that will collect the data returned by the request, can
    be NULL
    @param[in] timeout waiting timeout in ms (osWaitForever for blocking function, not recommended)
    @return 0 on success, HTTP error (<0) on failure
    */
    HTTPResult put( const char* url,
                    const IHTTPDataOut& dataOut,
                    IHTTPDataIn* pDataIn,
                    int timeout = HttpClienDefaultTimeout );  // Blocking

    /** Execute a DELETE request on the URL
    Blocks until completion
    @param[in] url : url on which to execute the request
    @param[in,out] pDataIn : pointer to an IHTTPDataIn instance that will collect the data returned by the request, can
    be NULL
    @param[in] timeout waiting timeout in ms (osWaitForever for blocking function, not recommended)
    @return 0 on success, HTTP error (<0) on failure
    */
    HTTPResult del( const char* url, IHTTPDataIn* pDataIn, int timeout = HttpClienDefaultTimeout );  // Blocking

    /** Get last request's HTTP response code
    @return The HTTP response code of the last request
    */
    int getHTTPResponseCode();

    /** Set the maximum number of automated redirections
    @param[in] i is the number of redirections. Values < 1 are
        set to 1.
    */
    void setMaxRedirections( int i = 1 );

    /** get the redirect location url
    @returns const char pointer to the url.
    */
    const char* getLocation() {
        return m_location;
    }

private:
    enum class HttpMethod { GET, POST, PUT, DELETE, HEAD };

    enum class AuthMethod { None, Basic, Bearer };

    HTTPResult connect( const char* url,
                        HttpMethod method,
                        IHTTPDataOut* pDataOut,
                        IHTTPDataIn* pDataIn,
                        int timeout );                                             // Execute request
    HTTPResult recv( char* buf, size_t minLen, size_t maxLen, size_t* pReadLen );  // 0 on success, err code on failure
    HTTPResult send( const char* buf, size_t len = 0 );                            // 0 on success, err code on failure
    HTTPResult parseURL( const char* url,
                         char* scheme,
                         size_t maxSchemeLen,
                         char* host,
                         size_t maxHostLen,
                         uint16_t* port,
                         char* path,
                         size_t maxPathLen );  // Parse URL
    void createauth( const char* user, const char* pwd, char* buf, int len );
    int base64enc( const char* input, unsigned int length, char* output, int len );

    HTTPResult receiveDataWithContentLength( char* buf,
                                             size_t trfLen,
                                             size_t recvContentLength,
                                             bool recvChunked,
                                             size_t maxChunkSize,
                                             IHTTPDataIn* pDataIn );

    HTTPResult receiveDataWithoutContentLength( char* buf,
                                                size_t trfLen,
                                                size_t recvContentLength,
                                                bool recvChunked,
                                                size_t maxChunkSize,
                                                IHTTPDataIn* pDataIn );

    ISocket& m_sock;

    int m_timeout = 0;

    AuthMethod m_authMethod = AuthMethod::None;
    const char* m_basicAuthUser;
    const char* m_basicAuthPassword;

    const char** m_customHeaders;
    size_t m_nCustomHeaders;
    int m_httpResponseCode;
    int m_maxredirections;
    char* m_location;
};

}  // namespace network
