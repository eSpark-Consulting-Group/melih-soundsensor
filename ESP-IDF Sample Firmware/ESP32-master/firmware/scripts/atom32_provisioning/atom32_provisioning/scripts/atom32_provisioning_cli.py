import argparse
from atom32_provisioning import app

def main():
    parser = argparse.ArgumentParser(
        description='Atom32 console provisioning tool')
    parser.add_argument(
        '-ep',
        '--esp_port',
        dest='esp32_port',
        help='ESP32 COM port',
        required=True)
    parser.add_argument(
        '-sp',
        '--zwave_sniffer_port',
        dest='zwave_sniffer_port',
        help='ZWave sniffer COM port',
        required=True)
    parser.add_argument(
        '-ai',
        '--application_image',
        dest='esp32_app',
        help='Esp32 application image',
        required=True)
    parser.add_argument(
        '-tai',
        '--test_application_image',
        dest='esp32_test_app',
        help='Esp32 test application image',
        required=True)
    parser.add_argument(
        '-bi',
        '--bootloader_image',
        dest='esp32_bootloader',
        help='Esp32 bootloader image',
        required=True)
    parser.add_argument(
        '-pi',
        '--partition_image',
        dest='esp32_partition_table',
        help='Esp32 partition table image',
        required=True)
    parser.add_argument(
        '-oi',
        '--ota_image',
        dest='esp32_ota_data',
        help='Esp32 ota data image',
        required=True)
    parser.add_argument(
        '-ii',
        '--id_image',
        dest='esp32_id',
        help='Esp32 ID partition image',
        required=True)
    parser.add_argument(
        '-key',
        '--aes256_key',
        dest='aes256_key',
        help='Esp32 firmware AES256 key',
        required=False)
    parser.add_argument(
        '-encrypt',
        '--encrypt_esp32',
        action='store_true',
        dest='encrypt_esp32',
        help='Burn flash encryption key and disable debug on ESP32',
        required=False)
    parser.add_argument(
        '-ssid',
        dest='ap_ssid',
        help='AP SSID',
        required=True)
    parser.add_argument(
        '-pass',
        dest='ap_pass',
        help='AP password',
        required=True),
    parser.add_argument(
        '-wrssi',
        type=int,
        dest='min_wifi_rssi',
        help='WiFi minimum RSSI (negative value)',
        required=True)
    parser.add_argument(
        '-zrssi',
        type=int,
        dest='min_zwave_rssi',
        help='ZWave minimum RSSI (positive value)',
        required=True)
    parser.add_argument(
        '-zregion',
        dest='zwave_region',
        help='ZWave region',
        required=True)
    parser.add_argument(
        '-server',
        dest='server',
        help='Factory provisioning server',
        required=True)
    args = parser.parse_args()
    theApp = app(esp32_port=args.esp32_port,
                 zwave_sniffer_port=args.zwave_sniffer_port,
                 esp32_app=args.esp32_app,
                 esp32_test_app=args.esp32_test_app,
                 esp32_bootloader=args.esp32_bootloader,
                 esp32_partition_table=args.esp32_partition_table,
                 esp32_ota_data=args.esp32_ota_data,
                 esp32_id=args.esp32_id,
                 aes256_key=args.aes256_key,
                 encrypt_esp32=args.encrypt_esp32,
                 ap_ssid=args.ap_ssid,
                 ap_pass=args.ap_pass,
                 min_wifi_rssi=args.min_wifi_rssi,
                 min_zwave_rssi=args.min_zwave_rssi,
                 zwave_region=args.zwave_region,
                 server = args.server)
    theApp.run()


if __name__ == '__main__':
    main()
