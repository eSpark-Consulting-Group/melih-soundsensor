#include <IDInfo.h>
#include <ZWaveGateway.h>
#include <FirmwareVersion.h>
#include "zwave_device_info.h"

namespace gateways {
namespace zwave {

namespace {

const uint8_t HardwareVersionUS = 0x01;
const uint8_t HardwareVersionEU = 0x02;
const uint8_t HardwareVersionDefault = 0x00;

const uint16_t ManufactirerId = 0x025B;

const uint16_t ProductTypeAtom = 0x01;
const uint16_t ProductTypePlugHub = 0x02;
const uint16_t ProductTypeWaterValve = 0x03;
const uint16_t ProductTypeDefault = 0x00;

const uint16_t ProductIdAtomRev1US = 0x01;
const uint16_t ProductIdAtomRev1EU = 0x02;
const uint16_t ProductIdAtomRev2US = 0x03;
const uint16_t ProductIdAtomRev2EU = 0x04;
const uint16_t ProductIdAtomRev3US = 0x05;
const uint16_t ProductIdAtomRev3EU = 0x06;

const uint16_t ProductIdPlugRev1US = 0x01;
const uint16_t ProductIdPlugRev1EU = 0x02;
const uint16_t ProductIdPlugRev2US = 0x03;
const uint16_t ProductIdPlugRev2EU = 0x04;
const uint16_t ProductIdPlugRev3US = 0x05;
const uint16_t ProductIdPlugRev3EU = 0x06;

const uint16_t ProductIdValveUS = 0x01;
const uint16_t ProductIdValveEU = 0x02;

const uint16_t ProductIdDefault = 0x00;

const uint16_t InstallerIconType500Series = 0x0500;
const uint16_t UserIconType500Series = 0x0500;
const uint16_t InstallerIconType700Series = 0x0500;
const uint16_t UserIconType700Series = 0x0500;

const uint16_t ChipFirmwareId500US = 0x0501;
const uint16_t ChipFirmwareId500EU = 0x0502;
const uint16_t ChipFirmwareId500Default = 0x0500;
const uint16_t ChipFirmwareId700US = 0x0701;
const uint16_t ChipFirmwareId700EU = 0x0702;
const uint16_t ChipFirmwareId700Default = 0x0700;

const uint8_t ZwaveChipType500 = 0x01;
const uint8_t ZwaveChipType700 = 0x02;

std::string getRegion() {
    auto& idInfo = core::IDInfo::get();
    const char* region = idInfo.zwaveRegion();
    if( !region ) {
        return std::string{};
    }
    return region;
}

uint8_t getZwaveChipType() {
    auto info = gateways::zwave::ZNet::get().getCommonInfo();
    if( !info ) {
        return ZwaveChipType500;
    }
    uint16_t chip_type;
    if( info->m_type < 0x07 ) {  // ZW_LIB_CONTROLLER_BRIDGE
        chip_type = ZwaveChipType500;
    }
    else {
        chip_type = ZwaveChipType700;
    }
    return chip_type;
}

}  // namespace

uint8_t getHardwareVersion() {
    const auto region = getRegion();
    return region == "EU" ? HardwareVersionEU : region == "US" ? HardwareVersionUS : HardwareVersionDefault;
}

uint16_t getManufacturerId() {
    return ManufactirerId;
}

uint16_t getProductType() {
#ifdef CONFIG_BOARD_TYPE_PLUG_HUB
    return ProductTypePlugHub;
#elif CONFIG_BOARD_WATER_VALVE
    return ProductTypeWaterValve;
#else
    return ProductTypeAtom;
#endif
}

uint16_t getProductId() {
    uint16_t product_id;
    const auto region = getRegion();
    const auto product_type = getProductType();
    const auto zwave_chip = getZwaveChipType();
    switch( product_type ) {
    case ProductTypeAtom:
#ifdef CONFIG_SPIRAM
        if( zwave_chip == ZwaveChipType700 )
            product_id = region == "US" ? ProductIdAtomRev2US : region == "EU" ? ProductIdAtomRev2EU : ProductIdDefault;
        else
            product_id = region == "US" ? ProductIdAtomRev3US : region == "EU" ? ProductIdAtomRev3EU : ProductIdDefault;
#else
        product_id = region == "US" ? ProductIdAtomRev1US : region == "EU" ? ProductIdAtomRev1EU : ProductIdDefault;
#endif
        break;
    case ProductTypePlugHub:
#ifdef CONFIG_SPIRAM
        if( zwave_chip == ZwaveChipType700 )
            product_id = region == "US" ? ProductIdPlugRev2US : region == "EU" ? ProductIdPlugRev2EU : ProductIdDefault;
        else
            product_id = region == "US" ? ProductIdPlugRev3US : region == "EU" ? ProductIdPlugRev3EU : ProductIdDefault;
#else
        product_id = region == "US" ? ProductIdPlugRev1US : region == "EU" ? ProductIdPlugRev1EU : ProductIdDefault;
#endif
        break;
    case ProductTypeWaterValve:
        product_id = region == "US" ? ProductIdValveUS : region == "EU" ? ProductIdValveEU : ProductIdDefault;
        break;
    }
    return product_id;
}

uint16_t getInstallerIconType() {
    return getZwaveChipType() == ZwaveChipType700 ? InstallerIconType700Series : InstallerIconType500Series;
}

uint16_t getUserIconType() {
    return getZwaveChipType() == ZwaveChipType700 ? UserIconType700Series : UserIconType500Series;
}

uint16_t getZwaveChipFirmwareId() {
    const auto region = getRegion();
    uint16_t chip_firmware_id;
    if( getZwaveChipType() == ZwaveChipType700 ) {
        chip_firmware_id =
            region == "EU" ? ChipFirmwareId700EU : region == "US" ? ChipFirmwareId700US : ChipFirmwareId700Default;
    }
    else {
        chip_firmware_id =
            region == "EU" ? ChipFirmwareId500EU : region == "US" ? ChipFirmwareId500US : ChipFirmwareId500Default;
    }
    return chip_firmware_id;
}

std::string getZwaveChipLocale() {
    return getRegion();
}

uint16_t getHubFirmwareId() {
    return getProductType();
}

uint16_t getZWaveAllianceLockingId() {
    // TODO: set custom id
    return getManufacturerId();
}

FirmwareVersion getZwaveChipFirmwareVersion() {
    auto info = gateways::zwave::ZNet::get().getCommonInfo();
    if( !info ) {
        return { 0, 0 };
    }
    std::string firmware_version = info->m_module_firmware_version;
    uint8_t version, sub_version;
    sscanf( firmware_version.c_str(), "%hhu.%hhu", &version, &sub_version );

    return { version, sub_version };
}

FirmwareVersion getHubFirmwareVersion() {
    return { FIRMWARE_VER_MAJOR, FIRMWARE_VER_MINOR };
}

}  // namespace zwave
}  // namespace gateways
