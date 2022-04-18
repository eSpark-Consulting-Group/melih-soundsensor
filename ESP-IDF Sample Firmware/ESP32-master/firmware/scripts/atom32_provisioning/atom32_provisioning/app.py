import serial
import subprocess
import time
import re
import os
import csv
import shutil
import pathlib as pt

from .esp32_firmware import Esp32Firmware
from .esp32 import Esp32
from .esp32_test import Esp32Test
from .exception import RuntimeException
from .id_info import IdInfo
from .id_partition import IdPartition
from .io_control import IoControl
from .utils import run_shell_command
import requests 
import json
import uuid
from copy import deepcopy
import logging as log

class app:

    __TEMP_DIRECTORY = 'temp'
    __AES256_KEY_FILENAME = 'aes256.key'
    __ID_PARTITION_IMAGE_FILENAME = 'id.bin'
    __DEVICE_LABEL_FILENAME = 'label.txt'
    __INVALID_TEST_PARAMETER_VALUE = -100;
    __USED_MACS_LIST_FILE = 'used_mac_list.csv'
    __MACS_LIST_FILE = 'mac_list.csv'

    def __init__(self,
                 esp32_port,
                 zwave_sniffer_port,
                 esp32_app,
                 esp32_test_app,
                 esp32_bootloader,
                 esp32_partition_table,
                 esp32_ota_data,
                 esp32_id,
                 aes256_key,
                 encrypt_esp32,
                 ap_ssid,
                 ap_pass,
                 min_wifi_rssi,
                 min_zwave_rssi,
                 zwave_region,
                 server,
                 ignore_button,
                 device_type):
        self.esp32_port = esp32_port
        self.zwave_sniffer_port = zwave_sniffer_port
        self.firmware = Esp32Firmware(
            app=esp32_app,
            test_app=esp32_test_app,
            bootloader=esp32_bootloader,
            partition_table=esp32_partition_table,
            ota_data=esp32_ota_data,
            id_data=esp32_id,
            aes256_key=aes256_key)
        self.esp32 = Esp32(
            serial_port=self.esp32_port,
            temp_directory=self.__TEMP_DIRECTORY)
        self.encrypt_esp32 = encrypt_esp32
        self.ap_ssid = ap_ssid
        self.ap_pass = ap_pass
        self.min_wifi_rssi = min_wifi_rssi
        self.min_zwave_rssi = min_zwave_rssi
        self.zwave_region = zwave_region
        self.server = server
        self.mac_address = None
        self.zwave_s2_dsk = None
        self.wifi_rssi = None
        self.zwave_rssi = None
        self.button_test = None
        self.id_info = None
        self.io_control = IoControl()
        self.io_control.setOk()
        log.basicConfig(filename='base.log', format='%(asctime)-15s | %(levelname)-8s | %(message)s', level=log.INFO)
        self.emulator_mode = os.path.isfile(self.__MACS_LIST_FILE)

        self.ignore_button = ignore_button
        self.device_type = device_type 
        if self.emulator_mode:
           print ("Emulator Mode")


    def run(self):
        while True:
            try:
                self.clean()
                if not (self.emulator_mode and self.ignore_button):
                   self.io_control.wait_for_press()
                self.io_control.setProgress()
                log.info('Start test')
                if self.main():
                    self.io_control.setOk()
                else:
                    self.io_control.setError()
                    log.error('Test failed')
            except Exception as ex:
                self.io_control.setError()
                log.error('Test failed. Exception: {}'.format(ex))
                pass

    def clean(self):
        self.__create_empty_temp_directory()
        self.mac_address = None
        self.zwave_s2_dsk = None
        self.wifi_rssi = None
        self.zwave_rssi = None
        self.button_test = None
        self.id_info = None


    def main(self):
        print('Start Esp32 test')

        try:
            firmware = deepcopy(self.firmware)
            if self.firmware.has_aes256_key():
                print('Encrypt firmware')
                firmware = firmware.encrypt(self.__TEMP_DIRECTORY)
            log.info("Flash test firmware")
            print('Flash test firmware')

            if not self.emulator_mode:
               self.esp32.disconnect()
               self.esp32.flash_firmware(firmware, test=True)
               self.esp32.connect()

            print('Read MAC address')
            log.info('Read MAC address')

            if self.emulator_mode:
                mac_str = self.__emulate_mac()
                if not mac_str:
                   raise RuntimeException()
            else:
                self.mac_address = self.esp32.get_mac_address()
                mac_str = ':'.join(['{i:02X}'.format(i=i) for i in self.mac_address])

            log.info('MAC address: {mac}'.format(mac=mac_str))
            print('MAC address: {mac}'.format(mac=mac_str))

            # Get ID info from server

            request_params = {'type': self.device_type, 'hardware_id': mac_str}
            print('Request ID info from server')
            log.info('Request ID data from server')
            result = requests.get(self.server, params = request_params)
            print('Request ID data from server - done')
            self.id_info = IdInfo()
            self.id_info.load_json(result.json())

            print('Start tests')
            log.info('Start test')
            if not self.emulator_mode:
                self.run_tests()

                if (self.wifi_rssi < self.min_wifi_rssi):# or (self.zwave_rssi < self.min_zwave_rssi) or (not self.button_test):
                   log.error('Test failed. Some values are lower then expected')
                   raise RuntimeException()

            # Generate ID partition

            print('Generate ID binary data')
            if not self.emulator_mode:
               id_image_file = str(pt.Path(self.__TEMP_DIRECTORY).joinpath(self.__ID_PARTITION_IMAGE_FILENAME))
               id_data = IdPartition()
               id_data.write(self.id_info, id_image_file)
               firmware.id_data = id_image_file

            if self.encrypt_esp32:
                if firmware.has_aes256_key():
                    # Firmware must be already encrypted
                    firmware.encrypt_image(
                        firmware.offsets['id_data'], firmware.id_data, firmware.id_data)
                else:
                    aes256_key = self.esp32.create_aes256_key()
                    aes256_key_file = pt.Path(self.__TEMP_DIRECTORY).joinpath(
                        self.__AES256_KEY_FILENAME)
                    aes256_key_file.write_bytes(aes256_key)
                    firmware = firmware.encrypt(
                        self.__TEMP_DIRECTORY, str(aes256_key_file))
                    self.esp32.secure(aes256_key_file)

            print('Flash user firmware')
            log.info('Flash user firmware')
            if not self.emulator_mode:
               self.esp32.disconnect()
               self.esp32.flash_firmware(firmware)

            # Generate file with info for a label

            if not self.emulator_mode:
                with open(self.__DEVICE_LABEL_FILENAME, 'w') as label:
                    label.write(str(self.id_info.id) + '\n')
                    label.write(':'.join(['{i:02X}'.format(i=i) for i in self.mac_address]) + '\n')
                    label.write('-'.join(['{value:05d}'.format(value=value) for value in self.zwave_s2_dsk]))

                    log.info('Print label')
                    command = ['python3', 'label_print.py', '192.168.100.157', str(9100), str(self.id_info.id), ''.join(['{value:05d}'.format(value=value) for value in self.zwave_s2_dsk])]
                    run_shell_command(command, 100)

            log.info('Send test results')
            self.__send_test_results(True)
            log.info('Test OK')
            print('Test OK')
            return True
        except RuntimeException:
            print('Test Failed')
            log.info('Send test results')
            self.__send_test_results(False)
            log.error('Test failed')
        return False

    def run_tests(self):

        test = Esp32Test()

        print('Test WiFi')
        try:
            if not self.emulator_mode:
               log.info('Start WiFi test')
               self.wifi_rssi = test.test_wifi(self.esp32, self.ap_ssid, self.ap_pass)
               log.info('WiFi rssi: {rssi}, min rssi: {min_rssi}'.format(rssi=self.wifi_rssi, min_rssi=self.min_wifi_rssi))
               print('WiFi rssi: {rssi}'.format(rssi = self.wifi_rssi))
               if self.wifi_rssi > self.min_wifi_rssi:
                  log.info('WiFi test OK')
                  print('WiFi test: OK')
               else:
                  log.error('WiFi test failed')
                  print('WiFi test: Failed')
            else:
                  log.info('WiFi test OK')
                  print('WiFi test: OK')

        except RuntimeException:
            log.error('WiFi test failed: exception')
            self.wifi_rssi = self.__INVALID_TEST_PARAMETER_VALUE
            print('WiFi test: Failed')

        print('Test ZWave')
        try:
            log.info('Start ZWave test')
            if not self.emulator_mode:
               self.zwave_rssi, self.zwave_home_id, self.zwave_s2_dsk = test.test_zwave(
               self.esp32, self.zwave_sniffer_port, self.id_info.zwave_region)
               print('ZWave rssi: {rssi}'.format(rssi = self.zwave_rssi))
               log.info('ZWave rssi: {rssi}, min rssi: {min_rssi}'.format(rssi=self.zwave_rssi, min_rssi=self.min_zwave_rssi))
               print('ZWave S2 DSK: {s2_dsk}'.format(
               s2_dsk = '-'.join(['{:05d}'.format(value) for value in self.zwave_s2_dsk])))
               if self.zwave_rssi > self.min_zwave_rssi:
                 log.info('ZWave test ok')
                 print('ZWave test: OK')
               else:
                 log.error('ZWave test faied')
                 print('ZWave test: Failed')
            else:
                log.info('ZWave test ok')
        except RuntimeException:
            self.zwave_rssi = self.__INVALID_TEST_PARAMETER_VALUE
            log.error('ZWave test faied: exception')
            print('ZWave test: Failed')

        print('Press button for more than 1 second')
        try:
            log.info('Start button test')
            self.button_test = test.test_button(self.esp32)
            if self.button_test:
                log.info('Button test OK')
                print('Button test: OK')
            else:
                log.error('Button test failed')
                print('Buton test: Failed')
        except RuntimeException:
            self.button_test = False
            log.error('Button test failed: exception')
            print('Button test: Failed')

    def __send_test_results(self, is_success):
        result = {}
        if self.id_info is None:
            return

        result['device_info'] = {'uuid': str(uuid.UUID(bytes = self.id_info.uuid))}
        wifi_result = False
        zwave_result = False
        button_result = False
        result['tests'] = []

        if self.emulator_mode:
           self.wifi_rssi = 90

        if self.wifi_rssi is not None:
           wifi_result = self.wifi_rssi > self.min_wifi_rssi
           result['tests'].append({'name': 'WiFi', 'result': wifi_result, 'value': self.wifi_rssi, 'threshold': self.min_wifi_rssi})

        if self.emulator_mode:
           result['tests'].append({'name': 'WiFi', 'result': wifi_result, 'value': self.wifi_rssi, 'threshold': self.min_wifi_rssi})

        if self.emulator_mode:
           self.zwave_rssi=90;

        if self.zwave_rssi is not None:
           zwave_result = self.zwave_rssi > self.min_zwave_rssi
           result['tests'].append({'name': 'ZWave', 'result': zwave_result, 'value': self.zwave_rssi, 'threshold': self.min_zwave_rssi})

        if self.emulator_mode:
            result['tests'].append({'name': 'ZWave', 'result': zwave_result, 'value': self.zwave_rssi, 'threshold': self.min_zwave_rssi})

        if self.emulator_mode:
           button_result = True
        else:
           if self.button_test is not None:
              button_result = self.button_test
        if self.emulator_mode: 
            result['tests'].append({'name': 'Button', 'result': button_result, 'value': int(True), 'threshold': 0})
        else:
           result['tests'].append({'name': 'Button', 'result': self.button_test, 'value': int(True), 'threshold': 0})

        result['provisioning_result'] = is_success and wifi_result and zwave_result and button_result
        requests.post(self.server, json = json.dumps(result))

    def __create_empty_temp_directory(self):
        tmp_dir = pt.Path(self.__TEMP_DIRECTORY)
        if tmp_dir.exists():
            if tmp_dir.is_dir():
                shutil.rmtree(str(tmp_dir))
            else:
                tmp_dir.unlink()
        tmp_dir.mkdir()

    def __emulate_mac(self):
       used_macs = []

       if os.path.isfile(self.__USED_MACS_LIST_FILE):
           with open(self.__USED_MACS_LIST_FILE) as csv_file:
              csv_reader = csv.reader(csv_file, delimiter=',')
              for row in csv_reader:
                  used_macs = used_macs + row

       if os.path.isfile(self.__MACS_LIST_FILE):
          with open(self.__MACS_LIST_FILE) as csv_file:
             csv_reader = csv.reader(csv_file, delimiter=',')
             for row in csv_reader:
               for item in row:
                   if item not in used_macs:
                       with open(self.__USED_MACS_LIST_FILE, 'a') as f:
                           writer = csv.writer(f)
                           writer.writerow([item])
                           print(item)
                           return item
       return ''
