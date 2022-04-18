import uuid

class IdInfo:

    def __init__(self):
        self.id = None
        self.uuid = None
        self.zwave_region = None
        self.default_wifi_ssid = None
        self.default_wifi_pass = None
        self.provisioning_server = None
        self.provisioning_token = None
        self.cloud_server = None
        self.ca_certificate = None
        self.ssl_private_key = None
        self.ssl_certificate = None

    def load_json(self, info):
        self.id = int(info['id'])
        self.uuid = uuid.UUID(info['uuid']).bytes
        self.zwave_region = info['zwave_region']
        self.default_wifi_ssid = info['default_wifi_ssid']
        self.default_wifi_pass = info['default_wifi_pass']
        self.provision_server = info['provision_server']
        self.provision_token = bytes(info['provisioning_token'], 'utf8')
        self.cloud_server = info['cloud_server']
        self.ca_certificate = info['ca_certificate']
        self.ssl_private_key = info['ssl_private_key']
        self.ssl_certificate = info['ssl_certificate']
        if 'product_name' in info.keys():
            self.product_name = info['product_name']
