set(SOURCES
	src/HTTPClient.cpp
    src/HTTPRequest.cpp
	src/ISocket.cpp
	src/TlsContext.cpp
	src/SSLSocket.cpp
	src/SSLServer.cpp
	src/TCPSocket.cpp
	src/WebSocket.cpp
	src/JsonSocket.cpp
	src/WebSocketServer.cpp
	src/TCPServer2.cpp
	
	src/HTTPData/HTTPJson.cpp
	src/HTTPData/HTTPJsonParsingData.cpp
	src/HTTPData/HTTPMap.cpp
	src/HTTPData/HTTPText.cpp
	src/HTTPData/HTTPXForm.cpp
	src/HTTPData/HTTPFile.cpp
)

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "include"
                       REQUIRES common freertos mdnsserver mbedtls hard esp_http_server esp_http_client peers_provision wifi_service2)
include(../config_flags_throw.cmake)
