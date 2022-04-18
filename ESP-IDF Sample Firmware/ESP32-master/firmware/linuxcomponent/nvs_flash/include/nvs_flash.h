#pragma once

#include "nvs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize NVS flash storage for the specified partition.
 *
 * @param[in]  partition_label   Label of the partition. Note that internally a reference to
 *                               passed value is kept and it should be accessible for future operations
 *
 * @return
 *      - ESP_OK if storage was successfully initialized.
 *      - ESP_ERR_NVS_NO_FREE_PAGES if the NVS storage contains no empty pages
 *        (which may happen if NVS partition was truncated)
 *      - ESP_ERR_NOT_FOUND if specified partition is not found in the partition table
 *      - one of the error codes from the underlying flash storage driver
 */
esp_err_t nvs_flash_init_partition( const char* partition_label );

/**
 * @brief Initialize NVS flash storage for the specified partition.
 *
 * @param[in]  partition_label   Label of the partition. Note that internally a reference to
 *                               passed value is kept and it should be accessible for future operations
 *
 * @return
 *      - ESP_OK if storage was successfully initialized.
 *      - ESP_ERR_NVS_NO_FREE_PAGES if the NVS storage contains no empty pages
 *        (which may happen if NVS partition was truncated)
 *      - ESP_ERR_NOT_FOUND if specified partition is not found in the partition table
 *      - one of the error codes from the underlying flash storage driver
 */
esp_err_t nvs_flash_init_partition( const char* partition_label );

/**
 * @brief Erase specified NVS partition
 *
 * This function erases all contents of specified NVS partition
 *
 * @param[in]  part_name    Name (label) of the partition to be erased
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_NOT_FOUND if there is no NVS partition with the specified name
 *        in the partition table
 */
esp_err_t nvs_flash_erase_partition( const char* part_name );

#ifdef __cplusplus
}  // extern "C"
#endif
