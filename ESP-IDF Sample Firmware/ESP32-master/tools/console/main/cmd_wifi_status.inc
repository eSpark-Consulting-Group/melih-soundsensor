/*
 * cmd_wifi_status.inc
 *
 *  Created on: Jan 18, 2018
 *      Author: boil
 */

static bool wifi_status()
{
    initialise_wifi();
    wifi_config_t wifi_config = { 0 };

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );

//typedef struct {
//    uint8_t bssid[6];                     /**< MAC address of AP */
//    uint8_t ssid[33];                     /**< SSID of AP */
//    uint8_t primary;                      /**< channel of AP */
//    wifi_second_chan_t second;            /**< second channel of AP */
//    int8_t  rssi;                         /**< signal strength of AP */
//    wifi_auth_mode_t authmode;            /**< authmode of AP */
//    wifi_cipher_type_t pairwise_cipher;   /**< pairwise cipher of AP */
//    wifi_cipher_type_t group_cipher;      /**< group cipher of AP */
//    uint32_t phy_11b:1;                   /**< bit: 0 flag to identify if 11b mode is enabled or not */
//    uint32_t phy_11g:1;                   /**< bit: 1 flag to identify if 11g mode is enabled or not */
//    uint32_t phy_11n:1;                   /**< bit: 2 flag to identify if 11n mode is enabled or not */
//    uint32_t phy_lr:1;                    /**< bit: 3 flag to identify if low rate is enabled or not */
//    uint32_t wps:1;                       /**< bit: 4 flag to identify if WPS is supported or not */
//    uint32_t reserved:27;                 /**< bit: 5..31 reserved */
//} wifi_ap_record_t;

	wifi_ap_record_t ap_info = {0};

    const esp_err_t ret_code = esp_wifi_sta_get_ap_info(&ap_info);
    LOG_U32(ret_code);

    if (ret_code == ESP_OK)
    {
    	printf("Wifi AP status:\n");

    	printf("AP MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
    			ap_info.bssid[0], ap_info.bssid[1], ap_info.bssid[2], ap_info.bssid[3], ap_info.bssid[4], ap_info.bssid[5]);

    	printf("AP SSID: '%s'\n", (const char*)ap_info.ssid);

    	printf("AP channel: '%d'\n", ap_info.primary);
    	printf("AP second channel: '%d'\n", (uint8_t)ap_info.second);

    	//    int8_t  rssi;                         /**< signal strength of AP */
    	//    wifi_auth_mode_t authmode;            /**< authmode of AP */
    	//    wifi_cipher_type_t pairwise_cipher;   /**< pairwise cipher of AP */
    	//    wifi_cipher_type_t group_cipher;      /**< group cipher of AP */
    	//    uint32_t phy_11b:1;                   /**< bit: 0 flag to identify if 11b mode is enabled or not */
    	//    uint32_t phy_11g:1;                   /**< bit: 1 flag to identify if 11g mode is enabled or not */
    	//    uint32_t phy_11n:1;                   /**< bit: 2 flag to identify if 11n mode is enabled or not */
    	//    uint32_t phy_lr:1;                    /**< bit: 3 flag to identify if low rate is enabled or not */
    	//    uint32_t wps:1;                       /**< bit: 4 flag to identify if WPS is supported or not */
    	//    uint32_t reserved:27;                 /**< bit: 5..31 reserved */

    }
    else
    if (ret_code == ESP_ERR_WIFI_CONN)
    {
    	ESP_LOGI(__func__, "The station interface is not initialized");
    }
    else
    if (ret_code == ESP_ERR_WIFI_NOT_CONNECT)
    {
    	ESP_LOGI(__func__, "The station is in disconnect status");
    }

    // ESP_ERROR_CHECK( ret_code );

    return 0;
}

/** Arguments used by 'status' function */
static struct tag_status_args {
    struct arg_end *end;
} status_args;

static int status(int argc, char** argv)
{
    int nerrors = arg_parse(argc, argv, (void**) &status_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, status_args.end, argv[0]);
        return 1;
    }
    ESP_LOGI(__func__, "Getting of status");

    bool status_got = wifi_status();
    if (status_got) {
        ESP_LOGW(__func__, "Getting of status failed");
        return 1;
    }
    ESP_LOGI(__func__, "Status is retrieved");
    return 0;
}

