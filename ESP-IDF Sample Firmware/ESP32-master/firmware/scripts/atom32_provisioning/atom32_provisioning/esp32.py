import serial
import subprocess
import time
import re
import shutil
import pathlib as pt
from .utils import run_shell_command, make_filename_in_directory
from .exception import RuntimeException, RebootException
import logging as log

class Esp32:
    __BAUD_RATE = 115200
    __READ_TIMEOUT_S = 1
    __REBOOT_PATTERN = b'ESP32RESTARTMAGICMESSAGE'
    __TIMEOUT_S = 20

    def __init__(self, serial_port, temp_directory):
        self.serial_port = serial_port
        self.temp_directory = temp_directory
        self.device = None

    def write(self, msg):
        if not self.device:
            raise RuntimeException()
        self.device.write(msg)

    def read(self, size):
        if not self.device:
            raise RuntimeException()
        return self.device.read(size)

    def create_aes256_key(self):
        if not self.device:
            raise RuntimeException()
        n_attempts = 5
        while n_attempts > 0:
            if not self.wait_for_prompt():
                return False
            self.device.write(b'CREATE_AES256_KEY\n')
            try:
                (aes256_key_str,) = self.get_reply(('AES256_KEY',))
            except (RuntimeException, serial.SerialException):
                n_attempts -= 1
                continue
            if len(aes256_key_str) != 64:
                n_attempts -= 1
                continue
            try:
                aes256_key = bytes.fromhex(aes256_key_str)
                assert(len(aes256_key) == 32)
                return aes256_key
            except ValueError:
                n_attempts -= 1
                continue
        raise RuntimeException


    def secure(self, key_filename):
        if not self.burn_encryption_key(key_filename):
            return False
        if not self.burn_efuse('FLASH_CRYPT_CNT', 0x01):
            return False
        if not self.burn_efuse('FLASH_CRYPT_CONFIG', 0x0F):
            return False
        if not self.burn_efuse('JTAG_DISABLE', 0x1):
            return False
        return True

    def burn_encryption_key(self, key_filename):
        TIMEOUT_S = 30
        command = ['espefuse.py',
                   '--port', self.serial_port,
                   'burn_key',
                   'flash_encryption',
                   key_filename,
                   '--do-not-confirm']
        run_shell_command(command, TIMEOUT_S)


    def burn_efuse(self, efuse, value):
        TIMEOUT_S = 30
        hex_format = '{:#x}'
        command = ['espefuse.py',
                   '--port', self.serial_port,
                   'burn_efuse',
                   efuse,
                   hex_format.format(value),
                   '--do-not-confirm']
        run_shell_command(command, TIMEOUT_S)


    def flash_firmware(self, firmware, test = False):
        FLASH_BAUD_RATE = 921600  # In case of problems use 460800
        TIMEOUT_S = 120
        hex_format = '{:#x}'
        command = ['esptool.py',
                   '-p', self.serial_port,
                   '--chip', 'esp32',
                   '-b', str(FLASH_BAUD_RATE),
                   'write_flash',
                   '--flash_mode', 'dio',
                   '--flash_size', 'detect',
                   '--flash_freq', '40m']

        if firmware.bootloader:
            command.extend(
                [hex_format.format(firmware.offsets['bootloader']), firmware.bootloader])
        if firmware.partition_table:
            command.extend(
                [hex_format.format(firmware.offsets['partition_table']), firmware.partition_table])
        if firmware.ota_data:
            command.extend(
                [hex_format.format(firmware.offsets['ota_data']), firmware.ota_data])
        app = firmware.app
        if test:
            app = firmware.test_app
        if app:
            command.extend(
                [hex_format.format(firmware.offsets['app']), app])
        if firmware.id_data:
            command.extend(
                [hex_format.format(firmware.offsets['id_data']), firmware.id_data])
        N_FLASH_ATTEMPTS = 5
        n_attempt = N_FLASH_ATTEMPTS
        while n_attempt > 0:
            try:
                log.info('Try to flash firmware. Attempt #{attempt}'.format(attempt = N_FLASH_ATTEMPTS - n_attempt + 1))
                run_shell_command(command, TIMEOUT_S)
                return
            except:
                log.error('Failed to flash firmware')
                n_attempt -= 1
                continue
        log.error('Failed to flash firmware. Giving up, raise exception')
        raise RuntimeException()



    def get_mac_address(self):
        if not self.device:
            raise RuntimeException()
        n_attempts = 5
        while n_attempts > 0:
            if not self.wait_for_prompt():
                raise RuntimeException()
            try:
                self.device.write(b'GET_MAC\n')
                (mac,) = self.get_reply(('MAC_ADDRESS',))
            except (RuntimeException, serial.SerialException):
                n_attempts -= 1
                self.disconnect()
                self.connect()
                continue
            if mac == '0' or len(mac) != 12:
                raise RuntimeException()
            mac_address = tuple(int(mac[i:i+2], 16) for i in range(0, 12, 2))
            return mac_address
        raise RuntimeException


    def wait_for_prompt(self):
        if not self.device:
            raise RuntimeException()
        start_time = time.monotonic()
        while time.monotonic() - start_time < self.__TIMEOUT_S:
            self.device.write(b'\n')
            buffer = self.device.read(256)
            if not buffer:
                continue
            if buffer[-1] == ord('#'):
                return True
        return False


    def get_reply(self, tokens):
        if not self.device:
            raise RuntimeException()
        buffer = bytearray()
        start_time = time.monotonic()
        result = {key : None for key in tokens}
        search_patterns = {key : bytes(key, 'utf8') + rb'\s\S+\s' for key in tokens}
        while time.monotonic() - start_time < self.__TIMEOUT_S:
            buffer.extend(self.device.read(256))
            for (key, pattern) in search_patterns.items():
                match = re.search(pattern, buffer)
                if match:
                    parts = str(buffer[match.start(): match.end()].decode('utf8')).split()
                    result[key] = parts[1]
            found_all = True
            for value in result.values():
                if not value:
                    found_all = False
                    break
            if found_all:
                return tuple(result.values())
            if re.search(self.__REBOOT_PATTERN, buffer):
                raise RebootException('Device reboot detected')
            # no more then 256 bytes needed
            buffer = buffer[-256:]
        raise RuntimeException('Can''t read values {}'.format(str(tokens)))


    def check_reply(self, *argv):
        if not self.device:
            raise RuntimeException()
        buffer = bytearray()
        pattern = bytearray()
        for arg in argv:
            pattern.extend(bytearray(arg, 'utf8'))
            pattern.extend(b'|')
        del pattern[-1]
        pattern = bytes(pattern)
        start_time = time.monotonic()
        while time.monotonic() - start_time < self.__TIMEOUT_S:
            buffer.extend(self.device.read(256))
            match = re.search(pattern, buffer)
            if match:
                return str(buffer[match.start(): match.end()].decode('utf8'))
            if re.search(self.__REBOOT_PATTERN, buffer):
                raise RebootException('Device reboot detected')
            # no more then 256 bytes needed
            buffer = buffer[-256:]
        raise RuntimeException('Can''t get reply')


    def connect(self):
        self.disconnect()
        self.device = serial.Serial(
            port=self.serial_port, baudrate=self.__BAUD_RATE, timeout=self.__READ_TIMEOUT_S)


    def disconnect(self):
        if self.device:
            self.device.close()
            self.device = None
