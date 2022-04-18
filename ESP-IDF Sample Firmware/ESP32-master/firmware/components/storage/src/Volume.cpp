#include "Volume.h"
#include "StoragePartitionLabel.h"
#include "Storage.h"

#include "BinaryFileStorageFactory.h"

#include <esp_ota_ops.h>
#include <esp_err.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <esp_littlefs.h>
#include <esp_partition.h>

#include <vector>
#include <string>

#include "DebugUtils.h"

namespace storage {

namespace Volume {

namespace {
bool isPartitionsValid() {
    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* boot = esp_ota_get_boot_partition();
    if( ( running == nullptr ) || ( boot == nullptr ) ) {
        return false;
    }

    if( running != boot ) {
        FUNC_LOGD( "The running partition (%s)", running->label );
        FUNC_LOGD( "The boot partition (%s)", boot->label );
        auto res = esp_ota_set_boot_partition( running );
        if( res != ESP_OK ) {
            FUNC_LOGE( "A setting of the other partition (%s) has been FAILED", running->label );
            return false;
        }
        FUNC_LOGI( "The partition (%s) has been set successfully", running->label );
    }

    return true;
}

bool tryInitializeNvs() {
    auto err = nvs_flash_init();
    if( err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND ) {
        if( ESP_OK != nvs_flash_erase() ) {
            return false;
        }
        err = nvs_flash_init();
    }
    return ESP_OK == err;
}

bool tryDeinitNvs() {
    auto ret = nvs_flash_deinit();
    if( ret != ESP_OK ) {
        FUNC_LOGE( "FLash initialization failed! Error code = %d", ret );
        return false;
    }
    return true;
}
}  // namespace

bool MountLittleFsVolume( const char* _partition_label, const char* _base_path, const bool _format_if_mount_failed ) {
    esp_vfs_littlefs_conf_t config;
    config.base_path = _base_path;
    config.partition_label = _partition_label;
    config.format_if_mount_failed = _format_if_mount_failed;

    return ESP_OK == esp_vfs_littlefs_register( &config );
}

bool UmountLittleFsVolume( const char* _partition_label ) {
    return ESP_OK == esp_vfs_littlefs_unregister( _partition_label );
}

bool Init() {
    if( !tryInitializeNvs() ) {
        ESP_LOGE( "Volume", "Can't initialize NVS" );
        return false;
    }

    if( !isPartitionsValid() ) {
        ESP_LOGE( "Volume", "Bad partitions" );
        return false;
    }

    if( !MountLittleFsVolume( PartitionLabel::STORAGE, m_basePath, true ) ) {
        ESP_LOGE( "Volume", "Can't mount %s partition as LittleFs", PartitionLabel::STORAGE );
        return false;
    }

    if( !MountLittleFsVolume( PartitionLabel::OPT, m_optPath, true ) ) {
        ESP_LOGE( "Volume", "Can't mount %s partition as LittleFs", PartitionLabel::OPT );
        return false;
    }

    if( !BinaryFileStorageFactory::checkAndCreateStorageDirectory() ) {
        ESP_LOGE( "Volume", "Can't create storage directory" );
        return false;
    }

    return true;
}

void Deinit() {
    storage::BinaryFileStorageFactory::get().reset();

    UmountLittleFsVolume( PartitionLabel::OPT );
    UmountLittleFsVolume( PartitionLabel::STORAGE );

    tryDeinitNvs();
}

bool FormatAllPartitions() {  // We must be sure that no one else read/write volume
    Deinit();
    const char* partitions[] = { PartitionLabel::STORAGE, PartitionLabel::OPT };
    for( const auto partition : partitions ) {
        if( ESP_OK != esp_littlefs_format( partition ) ) {
            FUNC_LOGE( "Failed to format %s parition", partition );
            return false;
        }
    }
    Init();
    return true;
}

}  // namespace Volume

}  // namespace storage
