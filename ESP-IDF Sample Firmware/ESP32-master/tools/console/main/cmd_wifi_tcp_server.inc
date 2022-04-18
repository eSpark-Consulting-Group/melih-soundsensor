/*
 * cmd_wifi_tcp_servert.inc
 *
 *  Created on: Jan 22, 2018
 *      Author: boil
 */

static int tcp_server_sock = -1;
static int tcp_server_client_sock = -1;

static esp_err_t tcp_server_create(const char* host, int port, int backlog)
{
    ESP_LOGI(__func__, "server socket....serverip:port=%s:%d\n", host, port);

    tcp_server_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (tcp_server_sock < 0) {
        show_socket_error_reason("create server", tcp_server_sock);
        return ESP_FAIL;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(host);

    ESP_LOGI(__func__, "creating the server...");

    if (bind(tcp_server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        show_socket_error_reason("bind_server", tcp_server_sock);
        closesocket(tcp_server_sock);
        return ESP_FAIL;
    }

    ESP_LOGI(__func__, "listening the socket ...");

    if (listen(tcp_server_sock, backlog) < 0) {
        show_socket_error_reason("listen_server", tcp_server_sock);
        closesocket(tcp_server_sock);
        return ESP_FAIL;
    }

    ESP_LOGI(__func__, "creating server success!");

    return ESP_OK;
}

static esp_err_t tcp_server_run()
{
	ESP_LOGI(__func__, "accepting the socket ...");

    struct sockaddr_in client_addr;
    unsigned int socklen = sizeof(client_addr);
    tcp_server_client_sock = accept(tcp_server_sock, (struct sockaddr *)&client_addr, &socklen);

    if (tcp_server_client_sock < 0) {
        show_socket_error_reason("accept_server", tcp_server_client_sock);
        closesocket(tcp_server_sock);
        return ESP_FAIL;
    }

    ESP_LOGI(__func__, "connect to server success!");

    return ESP_OK;
}

static esp_err_t tcp_server_send(const char* payload, int length)
{
    ESP_LOGI(__func__, "sending data (%d) bytes\n", length);

    ssize_t written = send(tcp_server_client_sock, payload, length, 0);

    if (written < 0)
    {
    	show_socket_error_reason("client connect", tcp_server_sock);
    	return ESP_FAIL;
    }

    if (written != length)
    {
    	ESP_LOGW(__func__, "sent (%d) byte(s) but buff length (%d) byte(s)\n", written, length);
    }
    else
    {
        ESP_LOGI(__func__, "sending to client success!");
    }

    return ESP_OK;
}


static esp_err_t tcp_server_recv(char* buffer, int length)
{
    ESP_LOGI(__func__, "reading data max (%d) bytes\n", length);

    ssize_t readLength = recv(tcp_server_client_sock, buffer, length, 0);

    if (readLength < 0)
    {
    	show_socket_error_reason("server connect", tcp_server_sock);
    	return ESP_FAIL;
    }

    if (readLength != length)
    {
    	ESP_LOGW(__func__, "got (%d) byte(s) but buff length (%d) byte(s)\n", readLength, length);
    }
    else
    {
        ESP_LOGI(__func__, "reading from client success!\n");
    }

    return ESP_OK;
}

static esp_err_t tcp_server_close_connection()
{
    ESP_LOGI(__func__, "closing the client socket\n");

    closesocket(tcp_server_client_sock);

    return ESP_OK;
}

static esp_err_t tcp_server_close()
{
    ESP_LOGI(__func__, "closing the server socket\n");

    closesocket(tcp_server_sock);

    return ESP_OK;
}

/** Arguments used by 'tcp_server' function */
static struct tag_tcp_server_args {
    struct arg_int *timeout;
    struct arg_str *host;
    struct arg_int *port;
    struct arg_int *backlog;
    struct arg_int *request_len;
    struct arg_str *payload;
    struct arg_end *end;
} tcp_server_args;


static void server_task(void *pvParameters)
{
    ESP_LOGI(__func__, "entering the TCP server task\n");

    if (ESP_OK == tcp_server_create(tcp_server_args.host->sval[0],
            tcp_server_args.port->ival[0],
            tcp_server_args.backlog->ival[0]))
    {
        if (ESP_OK == tcp_server_run())
        {
            char buff[tcp_server_args.request_len->ival[0]];

            if (ESP_OK == tcp_server_recv(buff, tcp_server_args.request_len->ival[0]))
            {
                if (ESP_OK == tcp_server_send(tcp_server_args.payload->sval[0],
                                              strlen(tcp_server_args.payload->sval[0])))
                {
                    /* EMPTY */
                }
            }
        }
    }

    tcp_server_close_connection();

    tcp_server_close();

    ESP_LOGI(__func__, "exiting the TCP server task\n");

    vTaskDelete( NULL );
}

static int tcp_server(int argc, char** argv)
{
    int nerrors = arg_parse(argc, argv, (void**) &tcp_server_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, tcp_server_args.end, argv[0]);
        return 1;
    }

    TaskHandle_t xHandle = NULL;
    BaseType_t xReturned =
            xTaskCreatePinnedToCore(server_task, "SE_T", 4096, NULL, 10, &xHandle, tskNO_AFFINITY);

    if (pdPASS != xReturned)
    {
        ESP_LOGE(__func__, "failed to create a TCP server task\n");
    }
    else
    {
        ESP_LOGI(__func__, "creating a TCP server task success\n");
    }
    return 0;
}
