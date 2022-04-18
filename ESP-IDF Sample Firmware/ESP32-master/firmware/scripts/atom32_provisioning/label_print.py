import hashlib
import os
import sys

from simple_zpl2 import ZPLDocument, NetworkPrinter

# Get arguments from parameters and validate them
return_error_code = 0

if len(sys.argv) != 5:
    return_error_code = 100
    exit(return_error_code)

# Printer IP, Printer Port, and Data for the QR Code
input_printer_ip = sys.argv[1]
input_printer_pt = int(sys.argv[2])
input_serial_txt = str(sys.argv[3])
input_dsk_number = sys.argv[4]


##############################
#    Generate QRCode Data    #
##############################

### Printer prefix letter file configuration
PRINTER_PREFIX_FILEPATH = "./printer_prefix.cfg"
PRINTER_PREFIX_DEFAULT  = "U"   # Default prefix on DSK for unknown printer (ie. when the prefix config file is unavailable or un-readable)

### Z-Wave device info configurations
ZWAVE_VERSION         = '01'    # '01'for Smart Start devices
ZWAVE_REQUESTED_KEYS  = '153'   # Extracting from excel sheet, could not find other reference to this field
ZWAVE_GENERIC_CLASS   = 0x02    # Generic Device Class and the Specific Device Class advertised in the node’s NIF
ZWAVE_SPECIFIC_CLASS  = 0x01
ZWAVE_INSTALLER_ICON  = 0x0100  # Specific Device Class advertised by the node’s Root Device Z-Wave Plus Info Report Command

ZWAVE_MANUFACTURER_ID = 0x025B  # Manufacturer ID advertised by the node in the Manufacturer Specific Report Command
ZWAVE_PRODUCT_TYPE    = 0x0002  # Product Type ID advertised by the node in the Manufacturer Specific Report Command
ZWAVE_PRODUCT_ID      = 0x0002  # Product ID advertised by the node in the Manufacturer Specific Report Command
ZWAVE_APP_VERSION     = 0x03    # Application Version and Application Sub Version advertised by the node in the Version Report Command
ZWAVE_APP_SUBVERSION  = 0x2D

## Label configurations
DSK1_X_ORIG = 70
DSK1_Y_ORIG = 128
DSK2_X_ORIG = 70
DSK2_Y_ORIG = 98

DSK_X_ORIG  = 235
DSK_Y_ORIG  = 18
QR_X_ORIG   = 230
QR_Y_ORIG   = 36
SN_X_ORIG   = 215
SN_Y_ORIG   = 165


class Tlv:

    type_critical = '00'
    length = '0'
    value = ''


def str_with_padding(value, length):

    value = str(value)
    if len(value) < length:
        value = ( (length - len(value)) ) * '0' + value

    return value


def get_printer_prefix():
    if os.path.exists(PRINTER_PREFIX_FILEPATH):
        with open(PRINTER_PREFIX_FILEPATH, 'r') as f:
            try:
                prefix_char = f.read(1)
                if len(prefix_char) > 0:
                    return prefix_char

            except:
                return PRINTER_PREFIX_DEFAULT

    return PRINTER_PREFIX_DEFAULT


def zwave_qr_code(serial, dsk):

    leadin = '90'
    version = ZWAVE_VERSION
    requested_keys = ZWAVE_REQUESTED_KEYS

### Custom TLVs ###

### Product Type
    product_type = Tlv()
    product_type.type_critical = '00'
    product_type.length = '10'

    device_class = (ZWAVE_GENERIC_CLASS << 8) + ZWAVE_SPECIFIC_CLASS
    device_class = str_with_padding(device_class, 5)

    icon_type = str_with_padding(ZWAVE_INSTALLER_ICON, 5)

    product_type.value = device_class + icon_type

### Product ID
    product_id = Tlv()
    product_id.type_critical = '02'
    product_id.length = '20'

    manufacturer_id = str_with_padding(ZWAVE_MANUFACTURER_ID, 5)
    adv_product_type = str_with_padding(ZWAVE_PRODUCT_TYPE, 5)
    adv_product_id = str_with_padding(ZWAVE_PRODUCT_ID, 5)
    app_version = (ZWAVE_APP_VERSION << 8) + ZWAVE_APP_SUBVERSION
    app_version = str_with_padding(app_version, 5)

    product_id.value = manufacturer_id + adv_product_type + adv_product_id + app_version

### UUID
    uuid = Tlv()
    uuid.type_critical = '06'
    uuid.length = '42'      # XXX: The length must account for 2 numbers of the pres_format plus 40 numbers of the UUID_data

    pres_format = '01' # The UUID_data represents 16 ASCII characters
    padded_serial = str_with_padding(serial, 16).encode('UTF-8')
    uuid_data = ''

    # XXX: According to the specification, the uuid data field does not store the serial directly, but rather represents every
    #      16 bits of data with 5 numbers, totalling 40 numbers for a 16-character serial

    for i in range(0, 16, 2):
        block = (padded_serial[i] << 8) + padded_serial[i + 1];
        uuid_data += str_with_padding(block, 5)

    uuid.value = pres_format + uuid_data

### Compute checksum

    tlvs = [product_type, product_id, uuid]

    code_tail = requested_keys + dsk + ''.join([tlv.type_critical + tlv.length + tlv.value for tlv in tlvs])
    sha1 = hashlib.sha1(code_tail.encode('UTF-8'))
    sha1 = sha1.digest()

    checksum = (int(sha1[0]) << 8) + int(sha1[1])
    checksum = str_with_padding(checksum, 5)

    return leadin + version + checksum + code_tail


input_qrcode_txt = zwave_qr_code(input_serial_txt, input_dsk_number)
print(input_qrcode_txt)



##############################
# Create the ZPL II document #
##############################

doc = ZPLDocument()
doc.add_field_origin(DSK2_X_ORIG, DSK2_Y_ORIG)
doc.add_zpl_raw('^A0A20,24')
doc.add_field_data('DSK ' + get_printer_prefix() + ':')

doc.add_field_origin(DSK1_X_ORIG, DSK1_Y_ORIG)
doc.add_zpl_raw('^A0A20,24')
doc.add_field_data(str(input_dsk_number)[0:5].replace("", " ")[1: -1])


# TODO Make dimensions proportional to an input scale on x and y in other words input parameter should be the print density
# TODO: Auto Pad Serial
# Add First 5 DSK Digits
# Todo Substr 5 and add spaces
doc.add_field_origin(DSK_X_ORIG, DSK_Y_ORIG)
doc.add_zpl_raw('^A0A20,24')
doc.add_field_data(str(input_dsk_number)[0:5].replace("", " ")[1: -1])

# Add QR Code
# TODO CENTER AND MAGNIFY ACCORDING TO SIZE
# bc = QR_Barcode(input_qrcode_txt, 2, 3, 'Q')
doc.add_field_origin(QR_X_ORIG, QR_Y_ORIG)
doc.add_zpl_raw('^BQ,2,3^FDQA,' + input_qrcode_txt + '^FS')
# doc.add_barcode(bc)

# TODO Make dimensions proportional to an input scale on x and y in other words input parameter should be the print density
# TODO: Auto Pad Serial
# Add Serial Number Label
doc.add_field_origin(SN_X_ORIG, SN_Y_ORIG)
doc.add_zpl_raw('^A0A20,24')
doc.add_field_data('S/N:' + input_serial_txt)

# TODO Use logging library instead
# Log ZPL command
# print('\nSingle-Line Output:\n' + doc.zpl_text.replace('\n', ' ').replace('\r', '').replace(' ^', '^'))
print('\nMulti-Line Output:\n' + doc.zpl_text)


##############################
#  Send ZPL to the printer   #
##############################

# noinspection PyBroadException
# Print the ZPL II code to the printer on the IP provided

try:
    prn = NetworkPrinter(input_printer_ip, input_printer_pt)
    # TODO: Timeout configurable via input parameters but having a default value
    prn.print_zpl(doc, 10)
    print("\nSuccess!")

except:
    print("\nError Trying to Print")
    return_error_code = 200
    exit(return_error_code)
