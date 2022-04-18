from .exception import RuntimeException, RebootException
from .zniffer import Zniffer
import time
import logging as log


class Esp32Test:
    __TIMEOUT_S = 15
    __N_TEST_ATTEMPTS = 5


    def test_wifi(self, esp32_device, ap_ssid, ap_pass):
        test_attempt = self.__N_TEST_ATTEMPTS
        while test_attempt > 0:
            log.info(
                'WiFi test attempt #{attempt}'.format(attempt=self.__N_TEST_ATTEMPTS - test_attempt + 1))
            if not esp32_device.wait_for_prompt():
                log.error('WiFi test: no prompt from device')
                raise RuntimeException()
            command = 'TEST_WIFI {ssid} {password}\n'.format(
                ssid=ap_ssid, password=ap_pass)
            esp32_device.write(bytearray(command, 'utf8'))
            try:
                (reply,) = esp32_device.get_reply(('WIFI_RSSI',))
            except (RebootException, RuntimeException):
                log.error('WiFi test: can''t get reply')
                test_attempt -= 1
                continue
            try:
                wifi_rssi = int(reply)
                if wifi_rssi == 0:
                    log.error('WiFi test: 0 reply')
                    test_attempt -= 1
                    continue
            except TypeError:
                log.error('WiFi test: wrong reply')
                # wrong reply, try again
                continue
            except ValueError:
                log.error('WiFi test: wrong reply')
                # wrong reply, try again
                continue
            else:
                return wifi_rssi
        log.error('WiFi test: test failed, raise exception')
        raise RuntimeException

    def test_zwave(self, esp32_device, zwave_sniffer_port, zwave_region):
        zniff = Zniffer()
        zniff.open(zwave_sniffer_port, zwave_region)
        test_attempt = self.__N_TEST_ATTEMPTS
        while test_attempt > 0:
            log.info('ZWave test attempt #{attempt}'.format(
                attempt=self.__N_TEST_ATTEMPTS - test_attempt + 1))
            if not esp32_device.wait_for_prompt():
                log.error('ZWave test: no prompt from device')
                return False
            esp32_device.write(b'TEST_ZWAVE\n')
            N_ID_READ_ATTEMPTS = 5
            id_read_attempt = N_ID_READ_ATTEMPTS
            home_id_string = ''
            s2_dsk_string = '' 
            while id_read_attempt > 0 :
                try:
                    (home_id_string, s2_dsk_string) = esp32_device.get_reply(('ZWAVE_HOME_ID','ZWAVE_S2_DSK'))
                except (RebootException, RuntimeException) as ex:
                    log.error('ZWave test: can''t get reply: {0}'.format(str(ex)))
                    break
                else:
                    S2_DSK_STR_SIZE = 47
                    if home_id_string == r'0' or len(s2_dsk_string) != S2_DSK_STR_SIZE:
                        log.error('ZWave test: can''t read Home ID')
                        home_id_string = ''
                        id_read_attempt -= 1
                        continue
                    s2_dsk = tuple([int(value) for value in s2_dsk_string.split('-')])
                    # Check if all s2_dsk parts are zeros
                    if sum(s2_dsk) == 0:
                        log.error('ZWave test: receive all 0 in DSK')
                        s2_dsk_string = ''
                        id_read_attempt -= 1
                        continue
                    break
            if not home_id_string or not s2_dsk_string:
                log.error('Failed to read Home ID or S2 DSK')
                test_attempt -= 1
                continue
            home_id = bytearray.fromhex(home_id_string)
            log.info('ZWave test: Home ID: {0}'.format(home_id_string))
            log.info('S2 DSK: {0}'.format(s2_dsk_string))
            start_time = time.monotonic()
            while time.monotonic() - start_time < self.__TIMEOUT_S:
                log.info('ZWave test: try to read message from zniffer')
                message = zniff.read(self.__TIMEOUT_S)
                if bool(message):
                    if message['home_id'] == home_id:
                        zwave_rssi = message['rssi']
                        return zwave_rssi, home_id, s2_dsk
            test_attempt -= 1
        log.error('ZWave test: test failed, raise exception')
        raise RuntimeException

    def test_button(self, esp32_device):
        if not esp32_device.wait_for_prompt():
            log.error('Button test: no prompt from device')
            raise RuntimeException
        esp32_device.write(b'TEST_BUTTON\n')
        result = esp32_device.check_reply('BUTTON_OK', 'BUTTON_ERROR')
        return result == 'BUTTON_OK'
