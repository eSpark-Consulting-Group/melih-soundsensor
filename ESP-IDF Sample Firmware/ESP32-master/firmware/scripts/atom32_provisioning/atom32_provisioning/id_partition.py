#!usr/bin/env python3

import uuid
from .buffer import Buffer, BufferView

class IdPartition:

    # Partition table
    ID_PARTITION_SIZE = 0x10000
    CONNECTION_ID_0_OFFSET = 0x0
    CONNECTION_ID_0_SIZE = 0x7000
    CONNECTION_ID_1_OFFSET = 0x7000
    CONNECTION_ID_1_SIZE = 0x7000
    HUB_ID_0_OFFSET = 0xE000
    HUB_ID_0_SIZE = 0x1000
    HUB_ID_1_OFFSET = 0xF000
    HUB_ID_1_SIZE = 0x1000

    # Partition data
    SN_OFFSET = 0
    SN_SIZE = 2
    VER_OFFSET = 2
    VER_SIZE = 2

    # Hub ID
    ID_OFFSET = 4
    ID_SIZE = 8
    UUID_OFFSET = 12
    UUID_SIZE = 16
    ZWAVE_REGION_OFFSET = 28
    ZWAVE_REGION_SIZE = 8
    DEFAULT_WIFI_SSID_OFFSET = 36
    DEFAULT_WIFI_SSID_MAX_SIZE = 32
    DEFAULT_WIFI_PASS_OFFSET = 68
    DEFAULT_WIFI_PASS_MAX_SIZE = 64
    NAME_OFFSET = 132
    NAME_SIZE = 128

    # Connection ID
    PROVISIONING_SERVER_URL_OFFSET = 4
    PROVISIONING_SERVER_URL_MAX_SIZE = 256
    PROVISIONING_SERVER_TOKEN_OFFSET = 260
    PROVISIONING_SERVER_TOKEN_SIZE = 256
    CLOUD_SERVER_URL_OFFSET = 516
    CLOUD_SERVER_URL_MAX_SIZE = 256
    CA_CERTIFICATE_OFFSET = 0x1000
    CA_CERTIFICATE_MAX_SIZE = 0x2000
    TLS_PRIVATE_KEY_OFFSET = 0x3000
    TLS_PRIVATE_KEY_MAX_SIZE = 0x1000
    TLS_PUBLIC_CERTIFICATE_OFFSET = 0x4000
    TLS_PUBLIC_CERTIFICATE_MAX_SIZE = 0x2000


    def __create_hub_id_partition(self, info, buffer, offset, size):
        assert(isinstance(buffer, Buffer))
        id_img = BufferView(buffer, offset=offset, size=size)
        id_img.write_int(self.SN_OFFSET, 0x00, self.SN_SIZE)
        id_img.write_int(self.VER_OFFSET, 0x02, self.VER_SIZE)
        id_img.write_int(self.ID_OFFSET, info.id, self.ID_SIZE)
        id_img.write_bytes(self.UUID_OFFSET, info.uuid)
        id_img.write_str(self.ZWAVE_REGION_OFFSET, info.zwave_region)
        id_img.write_str(self.DEFAULT_WIFI_SSID_OFFSET, info.default_wifi_ssid)
        id_img.write_str(self.DEFAULT_WIFI_PASS_OFFSET, info.default_wifi_pass)
        if hasattr(info, 'product_name'):
            id_img.write_str(self.NAME_OFFSET, info.product_name)
        else:
            id_img.write_str(self.NAME_OFFSET, '')

        
    def __create_connection_id_partition(self, info, buffer, offset, size):
        assert(isinstance(buffer, Buffer))
        conn_img = BufferView(buffer, offset=offset, size=size)
        conn_img.write_int(self.SN_OFFSET, 0x00, self.SN_SIZE)
        conn_img.write_int(self.VER_OFFSET, 0x01, self.VER_SIZE)
        conn_img.write_str(self.PROVISIONING_SERVER_URL_OFFSET, info.provision_server)
        conn_img.write_bytes(self.PROVISIONING_SERVER_TOKEN_OFFSET, info.provision_token)
        conn_img.write_str(self.CLOUD_SERVER_URL_OFFSET, info.cloud_server)
        conn_img.write_str(self.CA_CERTIFICATE_OFFSET, info.ca_certificate)
        conn_img.write_str(self.TLS_PRIVATE_KEY_OFFSET, info.ssl_private_key)
        conn_img.write_str(self.TLS_PUBLIC_CERTIFICATE_OFFSET, info.ssl_certificate)


    def write(self, info, filename):
        id_image = Buffer(size=self.ID_PARTITION_SIZE, value=0xFF)
        self.__create_hub_id_partition(info = info, buffer=id_image, offset=self.HUB_ID_0_OFFSET, size=self.HUB_ID_0_SIZE)
        self.__create_connection_id_partition(info = info, buffer=id_image, offset=self.CONNECTION_ID_0_OFFSET, size=self.CONNECTION_ID_0_OFFSET)
        with open(filename, 'wb') as id_file:
            assert( self.ID_PARTITION_SIZE == id_file.write(id_image.get()))    
