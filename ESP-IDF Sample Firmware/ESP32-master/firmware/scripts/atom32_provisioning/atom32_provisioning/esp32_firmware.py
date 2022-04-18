from .utils import run_shell_command, make_filename_in_directory
import pathlib as pt
from .exception import RuntimeException


class Esp32Firmware:

    def __init__(
            self,
            app=None,
            test_app=None,
            bootloader=None,
            partition_table=None,
            ota_data=None,
            id_data=None,
            aes256_key=None):
        self.app = app
        self.test_app = test_app
        self.bootloader = bootloader
        self.partition_table = partition_table
        self.ota_data = ota_data
        self.id_data = id_data
        self.aes256_key = aes256_key
        self.__partitions = None
        self.offsets = None
        if self.partition_table:
            self.__read_partitions()
            self.__create_offsets()


    def has_aes256_key(self):
        return self.aes256_key is not None


    def encrypt(self, temp_directory, aes256_key = None):
        
        if not aes256_key:
            aes256_key = self.aes256_key

        if not aes256_key or not self.offsets:
            raise RuntimeException
        
        tmp_dir = pt.Path(temp_directory)
        result = Esp32Firmware()
        result.__partitions = self.__partitions
        result.offsets = self.offsets
        result.aes256_key = aes256_key
        
        if self.bootloader:
            result.bootloader = make_filename_in_directory(
                tmp_dir, self.bootloader)
            self.encrypt_image(self.offsets['bootloader'],
                               self.bootloader,
                               result.bootloader)

        if self.partition_table:
            result.partition_table = make_filename_in_directory(
                tmp_dir, self.partition_table)
            self.encrypt_image(self.offsets['partition_table'],
                               self.partition_table,
                               result.partition_table)

        if self.app:
            result.app = make_filename_in_directory(tmp_dir, self.app)
            self.encrypt_image(self.offsets['app'],
                               self.app,
                               result.app)

        if self.test_app:
            result.test_app = make_filename_in_directory(
                tmp_dir, self.test_app)
            self.encrypt_image(self.offsets['app'],
                               self.test_app,
                               result.test_app)

        if self.id_data:
            result.id_data = make_filename_in_directory(tmp_dir, self.id_data)
            self.encrypt_image(self.offsets['id_data'],
                               self.id_data,
                               result.id_data)

        return result


    def encrypt_image(self, address, source, destination, aes256_key = None):

        if not aes256_key:
            aes256_key = self.aes256_key

        if not aes256_key or not self.offsets:
            raise RuntimeException

        clean_source = False
        if source == destination:
            pt.Path(source).replace(source + '.temp')
            clean_source = True
        TIMEOUT_S = 30
        hex_format = '{:#x}'
        command = ['espsecure.py',
                   'encrypt_flash_data',
                   '--keyfile', aes256_key,
                   '--address', hex_format.format(address),
                   '-o', destination,
                   source]
        run_shell_command(command, TIMEOUT_S)
        if clean_source:
            pt.Path(source).unlink()


    def __read_partitions(self):
        CHUNK_SIZE = 32
        self.__partitions = {}
        with open(self.partition_table, 'rb') as partitions_file:
            chunk = partitions_file.read(CHUNK_SIZE)
            while chunk and chunk[0] != 0xFF:
                offset = int.from_bytes(chunk[4:8], byteorder='little')
                size = int.from_bytes(chunk[8:12], byteorder='little')
                name = chunk[12:].decode('utf8').rstrip('\x00')
                self.__partitions[name] = (offset, size)
                chunk = partitions_file.read(CHUNK_SIZE)


    def __create_offsets(self):
        self.offsets = {'bootloader': 0x1000,
                        'partition_table': 0x8000,
                        'ota_data': 0xd000,
                        'app': self.__partitions['ota0'][0],
                        'id_data': self.__partitions['id'][0]}
