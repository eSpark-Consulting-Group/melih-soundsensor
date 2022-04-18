/*
 * cmd_wifi_scan.inc
 *
 *  Created on: Jan 18, 2018
 *      Author: boil
 */


static bool wifi_scan(int timeout_ms)
{
    initialise_wifi();
    wifi_scan_config_t wifi_scan_config = { 0 };
    wifi_scan_config.show_hidden = true;
    wifi_scan_config.scan_type = WIFI_SCAN_TYPE_PASSIVE;
    wifi_scan_config.scan_time.passive = timeout_ms;

    esp_err_t retcode = esp_wifi_set_mode(WIFI_MODE_STA);
    LOG_U32(retcode);
    ESP_ERROR_CHECK( retcode );

    retcode = esp_wifi_scan_start(&wifi_scan_config, 0);
    LOG_U32(retcode);
    ESP_ERROR_CHECK( retcode );

    const int32_t sleep_time = timeout_ms * (14+1);
    printf("falling into a sleep for %d ms ... ", sleep_time);

    int bits = xEventGroupWaitBits(wifi_event_group, GOT_IP_ADR_BIT,
            1, 1, sleep_time / portTICK_PERIOD_MS);
    xEventGroupClearBits(wifi_event_group, WIFI_SCANNED_BIT);

    printf("done.\n");

    uint16_t ap_count = 0;

    retcode = esp_wifi_scan_get_ap_num(&ap_count);
    LOG_U32(retcode);
    ESP_ERROR_CHECK( retcode );

    printf("AP count is (%d)\n", ap_count);

    if (ap_count)
    {
        ap_count += 2;
        wifi_ap_record_t ap_records [ap_count];

        memset(ap_records, 0x00, sizeof(ap_records));

        retcode = esp_wifi_scan_get_ap_records(&ap_count, ap_records);
        LOG_U32(retcode);
        ESP_ERROR_CHECK( retcode );

        printf("retrieved AP records count is (%d)\n", ap_count);

        for (uint16_t i = 0; i < ap_count; ++i)
        {
            printf("ssid[%d]='%s'\n" , i, ap_records[i].ssid);
        }

    }

    return (bits & WIFI_SCANNED_BIT) != 0;
}

/** Arguments used by 'scan' function */
static struct tag_scan_ap_args {
    struct arg_int *scan_time;
    struct arg_end *end;
} scan_ap_args;

static int scan_ap(int argc, char** argv)
{
    int nerrors = arg_parse(argc, argv, (void**) &scan_ap_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, scan_ap_args.end, argv[0]);
        return 1;
    }
    ESP_LOGI(__func__, "Scanning WiFi APs");

    bool scanned = wifi_scan(scan_ap_args.scan_time->ival[0]);
    if (!scanned) {
        ESP_LOGW(__func__, "Scanning timed out");
        return 1;
    }
    ESP_LOGI(__func__, "Scanned");
    return 0;
}
