import serial
import struct
import time
import re


class Zniffer:
    # The last byte is frequency region code
    __INIT_ZNIFFER_MESSAGE = b'\x23\x02\x01\x00'
    __FREQUENCY_REGION_CODE = {'EU': 0x00, 'US': 0x01, 'ANZ': 0x02, 'HK': 0x03,
                               'MY': 0x08, 'IN': 0x09, 'JP': 0x0A, 'RU': 0x1A,
                               'IL': 0x1B, 'KR': 0x1C, 'CN': 0x1D}
    __START_ZNIFFER_MESSAGE = b'\x23\x04\x00'
    __STOP_ZNIFFER_MESSAGE = b'\x23\x05\x00'
    __BAUD_RATE = 115200
    __TIMEOUT_S = 1
    __MESSAGE_SIZE = 1024

    def __init__(self):
        self.device = None
        self.buffer = bytearray()
        self.message = None

    def __del__(self):
        self.close()

    @property
    def region_codes(self):
        return [key for key in self.__FREQUENCY_REGION_CODE]

    def open(self, port, region_code):
        self.close()
        if region_code not in self.region_codes:
            raise ValueError
        self.device = serial.Serial(
            port=port, baudrate=self.__BAUD_RATE, timeout=self.__TIMEOUT_S)
        init_message = self.__INIT_ZNIFFER_MESSAGE[:-1] + bytes([self.__FREQUENCY_REGION_CODE[region_code]])
        self.device.write(init_message)
        self.device.write(self.__START_ZNIFFER_MESSAGE)
        return True

    def close(self):
        if self.device:
            self.device.write(self.__STOP_ZNIFFER_MESSAGE)
            self.device.close()
            self.device = None

    def read(self, timeout):
        self.message = {}
        start = time.monotonic()
        while time.monotonic() - start < timeout:
            self.buffer.extend(self.device.read(self.__MESSAGE_SIZE))
            if self.__parse_buffer():
                break
        return self.message

    def __parse_buffer(self):
        # find start
        DATA_FRAME_START = b'\x21\x01.{5}\x21\x03'
        HEADER_LENGTH = 10
        start_match = re.search(DATA_FRAME_START, self.buffer)
        if start_match == None:
            return False
        # throw everything before the start
        self.buffer = self.buffer[start_match.start():]
        # do we have enough data
        if len(self.buffer) < HEADER_LENGTH:
            return False
        data_length = self.buffer[9]
        self.message["rssi"] = self.buffer[6] - 5
        self.message["home_id"] = self.buffer[10:14]
        # clear full message
        if len(self.buffer) >= HEADER_LENGTH + data_length:
            self.buffer = self.buffer[HEADER_LENGTH + data_length:]
        return True
