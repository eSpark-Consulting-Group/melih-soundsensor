#include <stddef.h>

/**
 * @brief Configuration structure for esp_vfs_spiffs_register
 */
typedef struct {
    const char* base_path;       /*!< File path prefix associated with the filesystem. */
    const char* partition_label; /*!< Optional, label of SPIFFS partition to use. If set to NULL, first partition with
                                    subtype=spiffs will be used. */
    size_t max_files;            /*!< Maximum files that could be open at the same time. */
    bool format_if_mount_failed; /*!< If true, it will format the file system if it fails to mount. */
} esp_vfs_spiffs_conf_t;

/**
 * Register and mount SPIFFS to VFS with given path prefix.
 *
 * @param   conf                      Pointer to esp_vfs_spiffs_conf_t configuration structure
 *
 * @return
 *          - ESP_OK                  if success
 *          - ESP_ERR_NO_MEM          if objects could not be allocated
 *          - ESP_ERR_INVALID_STATE   if already mounted or partition is encrypted
 *          - ESP_ERR_NOT_FOUND       if partition for SPIFFS was not found
 *          - ESP_FAIL                if mount or format fails
 */
esp_err_t esp_vfs_spiffs_register( const esp_vfs_spiffs_conf_t* conf );

/**
 * Unregister and unmount SPIFFS from VFS
 *
 * @param partition_label  Optional, label of the partition to unregister.
 *                         If not specified, first partition with subtype=spiffs is used.
 *
 * @return
 *          - ESP_OK if successful
 *          - ESP_ERR_INVALID_STATE already unregistered
 */
esp_err_t esp_vfs_spiffs_unregister( const char* partition_label );

/**
 * Format the SPIFFS partition
 *
 * @param partition_label  Optional, label of the partition to format.
 *                         If not specified, first partition with subtype=spiffs is used.
 * @return
 *          - ESP_OK      if successful
 *          - ESP_FAIL    on error
 */
esp_err_t esp_spiffs_format( const char* partition_label );
