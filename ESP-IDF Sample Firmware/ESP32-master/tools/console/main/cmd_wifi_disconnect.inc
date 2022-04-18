/*
 * cmd_wifi_disconnect.inc
 *
 *  Created on: Jan 18, 2018
 *      Author: boil
 */


static bool wifi_disconnect(int timeout_ms)
{
    initialise_wifi();
    wifi_config_t wifi_config = { 0 };

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_disconnect() );

    int bits = xEventGroupWaitBits(wifi_event_group, GOT_IP_ADR_BIT,
            1, 1, timeout_ms / portTICK_PERIOD_MS);
    return (bits & LOST_IP_ADR_BIT) != 0;
}

/** Arguments used by 'disconnect' function */
static struct tag_disconnect_args {
    struct arg_int *timeout;
    struct arg_end *end;
} disconnect_args;

static int disconnect(int argc, char** argv)
{
    int nerrors = arg_parse(argc, argv, (void**) &disconnect_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, disconnect_args.end, argv[0]);
        return 1;
    }
    ESP_LOGI(__func__, "Disconnecting");

    bool disconnected = wifi_disconnect(disconnect_args.timeout->ival[0]);
    if (!disconnected) {
        ESP_LOGW(__func__, "Disconnection timed out");
        return 1;
    }
    ESP_LOGI(__func__, "Disconnected");
    return 0;
}
