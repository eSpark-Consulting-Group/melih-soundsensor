/*
 * cmd_wifi_tcp_client.inc
 *
 *  Created on: Jan 22, 2018
 *      Author: boil
 */

static int tcp_client_sock = -1;

static int get_socket_error_code(int socket)
{
    int result;
    u32_t optlen = sizeof(int);
    int err = getsockopt(socket, SOL_SOCKET, SO_ERROR, &result, &optlen);
    if (err == -1) {
        ESP_LOGE(__func__, "getsockopt failed:%s", strerror(err));
        return -1;
    }
    return result;
}

static int show_socket_error_reason(const char *str, int socket)
{
    int err = get_socket_error_code(socket);

    if (err != 0) {
        ESP_LOGW(__func__, "%s socket error %d %s", str, err, strerror(err));
    }

    return err;
}

static esp_err_t tcp_client_connect(const char* host, int port)
{
    ESP_LOGI(__func__, "client socket....serverip:port=%s:%d\n", host, port);

    tcp_client_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (tcp_client_sock < 0) {
        show_socket_error_reason("create client", tcp_client_sock);
        return ESP_FAIL;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(host);

    ESP_LOGI(__func__, "connecting to server...");

    if (connect(tcp_client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        show_socket_error_reason("client connect", tcp_client_sock);
        return ESP_FAIL;
    }

    ESP_LOGI(__func__, "connect to server success!");

    return ESP_OK;
}

static esp_err_t tcp_client_send(const char* payload, int length)
{
    ESP_LOGI(__func__, "sending data (%d) bytes\n", length);

    ssize_t written = send(tcp_client_sock, payload, length, 0);

    if (written < 0)
    {
    	show_socket_error_reason("client connect", tcp_client_sock);
    	return ESP_FAIL;
    }

    if (written != length)
    {
    	ESP_LOGW(__func__, "sent (%d) byte(s) but buff length (%d) byte(s)\n", written, length);
    }
    else
    {
    	ESP_LOGI(__func__, "sending to server success!");
    }

    return ESP_OK;
}


static esp_err_t tcp_client_recv(char* buffer, int length)
{
    ESP_LOGI(__func__, "reading data max (%d) bytes\n", length);

    ssize_t readLength = recv(tcp_client_sock, buffer, length, 0);

    if (readLength < 0)
    {
    	show_socket_error_reason("client connect", tcp_client_sock);
    	return ESP_FAIL;
    }

    if (readLength != length)
    {
    	ESP_LOGW(__func__, "got (%d) byte(s) but buff length (%d) byte(s)\n", readLength, length);
    }
    else
    {
    	ESP_LOGI(__func__, "reading from server success!");
    }

    return ESP_OK;
}

static esp_err_t tcp_client_close()
{
    ESP_LOGI(__func__, "closing the client socket\n");

    closesocket(tcp_client_sock);

    return ESP_OK;
}

/** Arguments used by 'tcp_client' function */
static struct tag_tcp_client_args {
    struct arg_int *timeout;
    struct arg_str *host;
    struct arg_int *port;
    struct arg_str *payload;
    struct arg_int *response_len;
    struct arg_end *end;
} tcp_client_args;

static int tcp_client(int argc, char** argv)
{
    int nerrors = arg_parse(argc, argv, (void**) &tcp_client_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, tcp_client_args.end, argv[0]);
        return 1;
    }

    if (ESP_OK == tcp_client_connect(tcp_client_args.host->sval[0],
    		tcp_client_args.port->ival[0]))
    {
    	if (ESP_OK == tcp_client_send(tcp_client_args.payload->sval[0],
    			strlen(tcp_client_args.payload->sval[0])))
    	{
    		char buff[tcp_client_args.response_len->ival[0]];

    		tcp_client_recv(buff, tcp_client_args.response_len->ival[0]);
    	}
    }

    tcp_client_close();

    return 0;
}
