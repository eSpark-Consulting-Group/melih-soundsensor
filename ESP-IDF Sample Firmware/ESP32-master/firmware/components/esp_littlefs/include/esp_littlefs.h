// Copyright 2015-2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <stdbool.h>
#include <lfs.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char* base_path;       /*!< File path prefix associated with the filesystem. */
    const char* partition_label; /*!< Optional, label of Littlefs partition to use. If set to NULL, first partition with
                                    subtype=spiffs will be used. */
    bool format_if_mount_failed; /*!< If true, it will format the file system if it fails to mount. */
} esp_vfs_littlefs_conf_t;

esp_err_t esp_vfs_littlefs_register( const esp_vfs_littlefs_conf_t* conf );

esp_err_t esp_vfs_littlefs_unregister( const char* partition_label );

bool esp_is_littlefs_mounted( const char* partition_label );

esp_err_t esp_littlefs_format( const char* partition_label );

esp_err_t esp_littlefs_info( const char* partition_label, size_t* total_bytes, size_t* used_bytes );

lfs_t* esp_littlefs_instance( const char* partition_label );

#ifdef __cplusplus
}
#endif
