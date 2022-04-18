#pragma once

namespace network {

static constexpr auto jsonNetworksPattern = "{"
                                            "\"status\": \"success\", "  // It's may be bad solution, but it's currently
                                                                         // (ATOM project, 25.03.2018) needed for legacy
                                            "\"networks\": [%s]"
                                            "}";

static constexpr auto jsonNetworkItemPattern =
    "{\"ssid\":\"%s\", \"signalLevel\":%d, \"open\":%s}, ";  // comma and space must be deleted after last item

static constexpr auto jsonConnectSuccessAnswer = "{"
                                                 "\"status\": "
                                                 "\"success\", "
                                                 "\"state\":\"connecting\""
                                                 "}";

static constexpr auto jsonAnswerSuccessStatusPattern = "{"
                                                       "\"status\": "
                                                       "\"success\", "
                                                       "\"state\":\"connected\", "
                                                       "\"SN\": \"%s\", "
                                                       "\"IP\": \"%s\", "
                                                       "\"SW\": \"0.01\", "
                                                       "\"MAC\": \"%s\""
                                                       "}";
static constexpr auto jsonStatusErrorAnswerPattern = "{"
                                                     "\"status\": \"%s\","
                                                     "\"error\": \"%s\","
                                                     "\"state\":\"%s\""
                                                     "}";
static constexpr auto http400Answer = "HTTP/1.1 400 Bad Request\r\n"
                                      "Access-Control-Allow-Origin: *\r\n"
                                      "User-Agent: %s\r\n"
                                      "Connection: close\r\n\r\n";

static constexpr auto http404Answer = "HTTP/1.1 404 Not Found\r\n"
                                      "Access-Control-Allow-Origin: *\r\n"
                                      "User-Agent: %s\r\n"
                                      "Connection: close\r\n\r\n";

static constexpr auto http401Answer = "HTTP/1.1 401 Not Found\r\n"
                                      "Access-Control-Allow-Origin: *\r\n"
                                      "User-Agent: %s\r\n"
                                      "Connection: close\r\n\r\n";

static constexpr auto http200Header = "HTTP/1.1 200 OK\r\n"
                                      "Access-Control-Allow-Origin: *\r\n"
                                      "User-Agent: %s\r\n";

static constexpr auto http500Answer = "HTTP/1.1 500 Internal Server Error\r\n"
                                      "User-Agent: %s\r\n\r\n";

static constexpr auto httpGetKeyword = "GET";
static constexpr auto httpPostKeyword = "POST";

static constexpr auto AP_HTTP_SERVER_NAME = "eZlo ATOM32 Server";

}  // namespace network
