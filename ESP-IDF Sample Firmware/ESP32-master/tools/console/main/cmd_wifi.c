/* Console example — WiFi commands

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "cmd_decl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "tcpip_adapter.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"

#include "debug_utils.h"

static EventGroupHandle_t wifi_event_group;
const int GOT_IP_ADR_BIT = BIT0;
const int LOST_IP_ADR_BIT = BIT1;
const int WIFI_SCANNED_BIT = BIT2;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, GOT_IP_ADR_BIT);
        xEventGroupClearBits(wifi_event_group, LOST_IP_ADR_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, GOT_IP_ADR_BIT);
        xEventGroupSetBits(wifi_event_group, LOST_IP_ADR_BIT);
        break;
    case SYSTEM_EVENT_SCAN_DONE:
        xEventGroupSetBits(wifi_event_group, WIFI_SCANNED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void initialise_wifi(void)
{
    esp_log_level_set("wifi", ESP_LOG_WARN);
    static bool initialized = false;
    if (initialized) {
        return;
    }
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    esp_err_t retcode = nvs_flash_init();
    LOG_U32(retcode);
    ESP_ERROR_CHECK( retcode );

    if (retcode == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        retcode = nvs_flash_erase();
        LOG_U32(retcode);
        ESP_ERROR_CHECK(retcode);

        retcode = nvs_flash_init();
        LOG_U32(retcode);
        ESP_ERROR_CHECK( retcode );
    }

    retcode = esp_wifi_init(&cfg);
    LOG_U32(retcode);
    ESP_ERROR_CHECK( retcode );

    retcode = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    LOG_U32(retcode);
    ESP_ERROR_CHECK( retcode );

    retcode = esp_wifi_set_mode(WIFI_MODE_NULL);
    LOG_U32(retcode);
    ESP_ERROR_CHECK( retcode );

    retcode = esp_wifi_start();
    LOG_U32(retcode);
    ESP_ERROR_CHECK( retcode );

    initialized = true;
}

#include "cmd_wifi_join.inc"
#include "cmd_wifi_disconnect.inc"
#include "cmd_wifi_status.inc"
#include "cmd_wifi_scan.inc"
#include "cmd_wifi_tcp_client.inc"
#include "cmd_wifi_tcp_server.inc"


void register_wifi()
{
    join_args.timeout = arg_int0(NULL, "timeout", "<t>", "Connection timeout, ms");
    join_args.timeout->ival[0] = 10000; // set default value
    join_args.ssid = arg_str1(NULL, NULL, "<ssid>", "SSID of AP");
    join_args.password = arg_str0(NULL, NULL, "<pass>", "PSK of AP");
    join_args.end = arg_end(2);

    const esp_console_cmd_t join_cmd = {
        .command = "join",
        .help = "Join WiFi AP as a station",
        .hint = NULL,
        .func = &wifi_connect,
        .argtable = &join_args
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&join_cmd) );

    //--------------------------------------------------------------------------------

    disconnect_args.timeout = arg_int0(NULL, "timeout", "<t>", "Disconnection timeout, ms");
    disconnect_args.timeout->ival[0] = 5000; // set default value
    disconnect_args.end = arg_end(2);

    const esp_console_cmd_t disconnect_cmd = {
        .command = "disconnect",
        .help = "Disconnect WiFi AP as a station",
        .hint = NULL,
        .func = &disconnect,
        .argtable = &disconnect_args
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&disconnect_cmd) );

    //--------------------------------------------------------------------------------

    status_args.end = arg_end(2);

    const esp_console_cmd_t status_cmd = {
        .command = "status",
        .help = "Getting of status of WiFi AP as a station",
        .hint = NULL,
        .func = &status,
        .argtable = &status_args
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&status_cmd) );

    //--------------------------------------------------------------------------------

    scan_ap_args.scan_time = arg_int0(NULL, "scan_time", "<t>", "Scan time per channel, ms");
    scan_ap_args.scan_time->ival[0] = 1000; // set default value
    scan_ap_args.end = arg_end(2);

    const esp_console_cmd_t scan_cmd = {
        .command = "scan",
        .help = "Scan WiFi APs",
        .hint = NULL,
        .func = &scan_ap,
        .argtable = &scan_ap_args
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&scan_cmd) );

    //--------------------------------------------------------------------------------

    tcp_client_args.timeout = arg_int0(NULL, "timeout", "<t>", "Disconnection timeout, ms");
    tcp_client_args.timeout->ival[0] = 5000; // set default value

    tcp_client_args.host = arg_str0(NULL, "host", "<h>", "Destination host name or IP address");
    tcp_client_args.host->sval[0] = "192.168.10.175"; // set default value

    tcp_client_args.port = arg_int0(NULL, "port", "<p>", "Destination port number");
    tcp_client_args.port->ival[0] = 2345; // set default value

    tcp_client_args.payload = arg_str0(NULL, "payload", "<d>", "Data to be sent");
    tcp_client_args.payload->sval[0] = "123456789"; // set default value

    tcp_client_args.response_len = arg_int0(NULL, "response_len", "<p>", "Expected response length");
    tcp_client_args.response_len->ival[0] = 6; // set default value

    tcp_client_args.end = arg_end(2);

    const esp_console_cmd_t tcp_client_cmd = {
        .command = "tcp_client",
        .help = "Connects to server socket and sends data",
        .hint = NULL,
        .func = &tcp_client,
        .argtable = &tcp_client_args
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&tcp_client_cmd) );

    //--------------------------------------------------------------------------------

    tcp_server_args.timeout = arg_int0(NULL, "timeout", "<t>", "Disconnection timeout, ms");
    tcp_server_args.timeout->ival[0] = 5000; // set default value

    tcp_server_args.host = arg_str0(NULL, "host", "<h>", "Destination host name or IP address");
    tcp_server_args.host->sval[0] = "192.168.11.130"; // set default value

    tcp_server_args.port = arg_int0(NULL, "port", "<p>", "Destination port number");
    tcp_server_args.port->ival[0] = 2345; // set default value

    tcp_server_args.backlog = arg_int0(NULL, "backlog", "<t>", "Disconnection timeout, ms");
    tcp_server_args.backlog->ival[0] = 5000; // set default value

    tcp_server_args.request_len = arg_int0(NULL, "request_len", "<p>", "Expected request length");
    tcp_server_args.request_len->ival[0] = 6; // set default value

    tcp_server_args.payload = arg_str0(NULL, "payload", "<d>", "Data to be sent");
    tcp_server_args.payload->sval[0] = "123456789"; // set default value

    tcp_server_args.end = arg_end(2);

    const esp_console_cmd_t tcp_server_cmd = {
        .command = "tcp_server",
        .help = "Connects to server socket and sends data",
        .hint = NULL,
        .func = &tcp_server,
        .argtable = &tcp_server_args
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&tcp_server_cmd) );
}
