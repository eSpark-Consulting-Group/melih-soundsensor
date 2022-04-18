from atom32_provisioning import app


def main():
    theApp = app(esp32_port='/dev/ttyUSB0',
                 zwave_sniffer_port='/dev/ttyACM0',
                 esp32_app="firmware/atom32.bin",
                 esp32_test_app="firmware/atom32_factory_test.bin",
                 esp32_bootloader="firmware/bootloader.bin",
                 esp32_partition_table="firmware/partition-table.bin",
                 esp32_ota_data="firmware/ota_data_initial.bin",
                 esp32_id="firmware/id.bin",
                 aes256_key=None,#"flash_encryption_key.bin",
                 encrypt_esp32=False,
                 ap_ssid='eZLO_Atom32',
                 ap_pass='smart16_inHouse',
                 min_wifi_rssi=-70,
                 min_zwave_rssi=40,
                 zwave_region='US',
                 server = 'http://atom32-provisioner-d.local:5000/provisioning/api/v1',
                 ignore_button = False, # Set automatic or manual mode (True or False) (only for emulator) by default manual mode.
                 device_type = "Atom32") # Set device type ("Atom32" or "PlugHub") by default "Atom32"
    theApp.run()

if __name__ == '__main__':
    main()
