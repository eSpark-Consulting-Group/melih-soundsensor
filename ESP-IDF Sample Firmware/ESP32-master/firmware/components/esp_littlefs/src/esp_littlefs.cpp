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

#include "esp_littlefs.h"
#include <lfs.h>
#include <esp_log.h>
#include <esp_partition.h>
#include <esp_spi_flash.h>
#include <esp_image_format.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/lock.h>
#include <esp_vfs.h>
#include <rom/spi_flash.h>
#include <string>
#include <algorithm>
#include <mutex>
#include <sdkconfig.h>

#define CHECK_FILE_DESCRIPTOR( _FD )                                  \
    if( 0 > ( _FD ) || CONFIG_ESP_VFS_LITTLEFS_MAX_FILES <= ( _FD ) ) \
    return -1

#define DECLARE_efs_CONTEXT_AND_HANDLE_LOCK( _CTX )  \
    esp_littlefs_t* efs = (esp_littlefs_t*)( _CTX ); \
    std::lock_guard<std::mutex> lock( efs->mutex )

#define CHECK_FILE_IS_IN_USE( _FD ) \
    if( !efs->files[_FD].is_used )  \
    return -1

#define CHECK_RET_CODE( _RET_CODE )                 \
    if( 0 > _RET_CODE ) {                           \
        errno = littlefs_res_to_errno( _RET_CODE ); \
        return -1;                                  \
    }

namespace {

const char* TAG = "LittleFs";

struct esp_littlefs_file_t {
    bool is_used;
    lfs_file_t file;
#if !CONFIG_ESP_LITTLEFS_MALLOC
    lfs_file_config conf;
    char fbuf[CONFIG_ESP_LITTLEFS_CACHE_SIZE];
#endif
    esp_littlefs_file_t()
    : is_used( false ) {
#if !CONFIG_ESP_LITTLEFS_MALLOC
        conf = {};
        conf.buffer = fbuf;
#endif
    }
};

typedef struct {
    bool is_used;
    lfs_t fs;                             /*!< Handle to the underlying SPIFFS */
    const esp_partition_t* partition;     /*!< The partition on which SPIFFS is located */
    char base_path[ESP_VFS_PATH_MAX + 1]; /*!< Mount point */
    bool by_label;                        /*!< Partition was mounted by label */
    struct lfs_config cfg;                /*!< SPIFFS Mount configuration */
#if !CONFIG_ESP_LITTLEFS_MALLOC
    uint8_t read_buf[CONFIG_ESP_LITTLEFS_READ_BUF_SIZE] __attribute__( ( aligned( 8 ) ) );
    uint8_t prog_buf[CONFIG_ESP_LITTLEFS_PROG_BUF_SIZE] __attribute__( ( aligned( 8 ) ) );
    uint8_t lookahead_buf[CONFIG_ESP_LITTLEFS_LOOKAHEAD_BUF_SIZE] __attribute__( ( aligned( 8 ) ) );
#endif
    std::array<esp_littlefs_file_t, CONFIG_ESP_VFS_LITTLEFS_MAX_FILES> files;
    bool is_mounted;
    int ret_code;
    std::mutex mutex;
} esp_littlefs_t;

typedef struct {
    DIR dir;         /*!< VFS DIR struct */
    lfs_dir_t d;     /*!< LittleFs DIR struct */
    struct dirent e; /*!< Last open dirent */
    // long offset;       /*!< Offset of the current dirent */
    std::string path; /*!< Requested directory name */
} vfs_littlefs_dir_t;

int vfs_littlefs_open( void* ctx, const char* path, int flags, int mode );
ssize_t vfs_littlefs_write( void* ctx, int fd, const void* data, size_t size );
ssize_t vfs_littlefs_read( void* ctx, int fd, void* dst, size_t size );
int vfs_littlefs_close( void* ctx, int fd );
int vfs_littlefs_fsync( void* ctx, int fd );
off_t vfs_littlefs_lseek( void* ctx, int fd, off_t offset, int mode );
int vfs_littlefs_fstat( void* ctx, int fd, struct stat* st );
int vfs_littlefs_stat( void* ctx, const char* path, struct stat* st );
int vfs_littlefs_unlink( void* ctx, const char* path );
int vfs_littlefs_link( void* ctx, const char* n1, const char* n2 );
int vfs_littlefs_rename( void* ctx, const char* src, const char* dst );
DIR* vfs_littlefs_opendir( void* ctx, const char* name );
int vfs_littlefs_closedir( void* ctx, DIR* pdir );
struct dirent* vfs_littlefs_readdir( void* ctx, DIR* pdir );
int vfs_littlefs_readdir_r( void* ctx, DIR* pdir, struct dirent* entry, struct dirent** out_dirent );
long vfs_littlefs_telldir( void* ctx, DIR* pdir );
void vfs_littlefs_seekdir( void* ctx, DIR* pdir, long offset );
int vfs_littlefs_mkdir( void* ctx, const char* name, mode_t mode );
int vfs_littlefs_rmdir( void* ctx, const char* name );
// int vfs_littlefs_utime(void *ctx, const char *path, const struct utimbuf *times);

esp_littlefs_t _efs[CONFIG_ESP_VFS_LITTLEFS_MAX_PARTITIONS] = {};

esp_err_t esp_littlefs_by_label( const char* label, int* index ) {
    int i;
    esp_littlefs_t* p;
    for( i = 0; i < CONFIG_ESP_VFS_LITTLEFS_MAX_PARTITIONS; i++ ) {
        p = &_efs[i];
        if( p->is_used ) {
            if( !label && !p->by_label ) {
                *index = i;
                return ESP_OK;
            }
            if( label && p->by_label && strncmp( label, p->partition->label, 17 ) == 0 ) {
                *index = i;
                return ESP_OK;
            }
        }
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t esp_littlefs_get_empty( int* index ) {
    int i;
    for( i = 0; i < CONFIG_ESP_VFS_LITTLEFS_MAX_PARTITIONS; i++ ) {
        if( !_efs[i].is_used ) {
            *index = i;
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

#define RET_SUCCESS ( 0 )
#define RET_FAIL ( -1 )

int littlefs_api_read( const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size ) {
    esp_littlefs_t* context = (esp_littlefs_t*)( c->context );
    lfs_off_t address = block * c->block_size + off;

    esp_err_t err = esp_partition_read( context->partition, address, buffer, size );
    if( err ) {
        ESP_LOGE( TAG, "failed to read addr %08x, size %08x, err %d", address, size, err );
        return RET_FAIL;
    }

    return RET_SUCCESS;
}

int littlefs_api_prog( const struct lfs_config* c,
                       lfs_block_t block,
                       lfs_off_t off,
                       const void* buffer,
                       lfs_size_t size ) {
    esp_littlefs_t* context = (esp_littlefs_t*)( c->context );
    lfs_off_t address = block * c->block_size + off;

    esp_err_t err = esp_partition_write( context->partition, address, buffer, size );
    if( err ) {
        ESP_LOGE( TAG, "failed to write addr %08x, size %08x, err %d", address, size, err );
        return -1;
    }

    return RET_SUCCESS;
}

int littlefs_api_erase( const struct lfs_config* c, lfs_block_t block ) {
    esp_littlefs_t* context = (esp_littlefs_t*)( c->context );
    lfs_off_t address = block * c->block_size;

    esp_err_t err = esp_partition_erase_range( context->partition, address, c->block_size );
    if( err ) {
        ESP_LOGE( TAG, "failed to erase addr %08x, size %08x, err %d", address, c->block_size, err );
        return RET_FAIL;
    }

    return RET_SUCCESS;
}

int littlefs_api_sync( const struct lfs_config* c ) {
    return RET_SUCCESS;
}

esp_err_t esp_littlefs_init( const esp_vfs_littlefs_conf_t* conf, int& index ) {
    // find if such partition is already mounted
    if( esp_littlefs_by_label( conf->partition_label, &index ) == ESP_OK ) {
        return ESP_ERR_INVALID_STATE;
    }

    if( esp_littlefs_get_empty( &index ) != ESP_OK ) {
        ESP_LOGE( TAG, "max mounted partitions reached" );
        return ESP_ERR_INVALID_STATE;
    }
    else
        ESP_LOGI( TAG, "found free littlefs partition index:%i", index );

    uint32_t flash_page_size = g_rom_flashchip.sector_size;
    uint32_t log_page_size = CONFIG_ESP_LITTLEFS_PAGE_SIZE;
    if( log_page_size % flash_page_size != 0 ) {
        ESP_LOGE( TAG, "ESP_LITTLEFS_PAGE_SIZE is not multiple of flash chip page size (%d)", flash_page_size );
        return ESP_ERR_INVALID_ARG;
    }

    esp_partition_subtype_t subtype =
        conf->partition_label ? ESP_PARTITION_SUBTYPE_ANY : ESP_PARTITION_SUBTYPE_DATA_SPIFFS;
    const esp_partition_t* partition =
        esp_partition_find_first( ESP_PARTITION_TYPE_DATA, subtype, conf->partition_label );
    if( !partition ) {
        ESP_LOGE( TAG, "spiffs(for littlefs) partition could not be found" );
        return ESP_ERR_NOT_FOUND;
    }

    if( partition->encrypted ) {
        ESP_LOGE( TAG, "spiffs(for littlefs) can not run on encrypted partition" );
        return ESP_ERR_INVALID_STATE;
    }

    esp_littlefs_t* efs = &_efs[index];

    efs->cfg.context = efs;

    efs->cfg.erase = littlefs_api_erase;
    efs->cfg.read = littlefs_api_read;
    efs->cfg.prog = littlefs_api_prog;
    efs->cfg.sync = littlefs_api_sync;

    efs->cfg.read_size = CONFIG_ESP_LITTLEFS_READ_BUF_SIZE;
    efs->cfg.prog_size = CONFIG_ESP_LITTLEFS_PROG_BUF_SIZE;
    efs->cfg.lookahead_size = CONFIG_ESP_LITTLEFS_LOOKAHEAD_BUF_SIZE;
    efs->cfg.cache_size = CONFIG_ESP_LITTLEFS_CACHE_SIZE;
#if !CONFIG_ESP_LITTLEFS_MALLOC
    efs->cfg.read_buffer = efs->read_buf;
    efs->cfg.prog_buffer = efs->prog_buf;
    efs->cfg.lookahead_buffer = efs->lookahead_buf;
#endif
    efs->cfg.block_size = log_page_size;
    efs->cfg.block_count = partition->size / log_page_size;

    efs->by_label = conf->partition_label != NULL;
    efs->partition = partition;

    efs->ret_code = lfs_mount( &efs->fs, &efs->cfg );
    if( 0 != efs->ret_code && conf->format_if_mount_failed ) {
        ESP_LOGW( TAG, "mount failed, %i. formatting...", efs->ret_code );
        efs->ret_code = lfs_format( &efs->fs, &efs->cfg );
        if( 0 != efs->ret_code ) {
            ESP_LOGE( TAG, "format failed, %i", efs->ret_code );
            return ESP_FAIL;
        }
        ESP_LOGI( TAG, "format successfull" );
        efs->ret_code = lfs_mount( &efs->fs, &efs->cfg );
    }
    if( 0 != efs->ret_code ) {
        ESP_LOGE( TAG, "mount failed, %i", efs->ret_code );
        return ESP_FAIL;
    }
    ESP_LOGI( TAG, "mount successfull" );
    efs->is_mounted = true;
    return ESP_OK;
}

int littlefs_res_to_errno( int fr ) {
    switch( fr ) {
    case LFS_ERR_OK:
        return 0;
    // case LFS_ERR_IO :          // Error during device operation
    //    return ENODEV;
    // case LFS_ERR_CORRUPT :     // Corrupted
    //    return ENODEV;
    case LFS_ERR_NOENT:  // No directory entry
        return ENOENT;   /* No such file or directory */
    // case LFS_ERR_EXIST :       // Entry already exists
    //    return EBADF;
    case LFS_ERR_NOTDIR:    // Entry is not a dir
        return ENOTDIR;     /* Not a directory */
    case LFS_ERR_ISDIR:     // Entry is a dir
        return EISDIR;      /* Is a directory */
    case LFS_ERR_NOTEMPTY:  // Dir is not empty
        return ENOTEMPTY;   /* Directory not empty */
    case LFS_ERR_BADF:      // Bad file number
        return EBADF;       /* Bad file number */
    case LFS_ERR_FBIG:      // File too large
        return EFBIG;       /* File too large */
    case LFS_ERR_INVAL:     // Invalid parameter
        return EINVAL;      /* Invalid argument */
    case LFS_ERR_NOSPC:     // No space left on device
        return ENOSPC;      /* No space left on device */
    case LFS_ERR_NOMEM:     // No more memory available
        return ENOMEM;      /* Not enough space */
    // case LFS_ERR_NOATTR :      // No data/attr available
    //    return EROFS;
    case LFS_ERR_NAMETOOLONG:  // File name too long
        return ENAMETOOLONG;   /* File or path name too long */
    default:
        return EIO;
    }
}

int littlefs_mode_conv( int m ) {
    int res = 0;

    int acc_mode = m & O_ACCMODE;
    if( acc_mode == O_RDONLY ) {
        res |= LFS_O_RDONLY;
    }
    else if( acc_mode == O_WRONLY ) {
        res |= LFS_O_WRONLY;
    }
    else if( acc_mode == O_RDWR ) {
        res |= LFS_O_RDWR;
    }

    if( m & O_CREAT )
        res |= LFS_O_CREAT;
    if( m & O_EXCL )
        res |= LFS_O_EXCL;
    if( m & O_TRUNC )
        res |= LFS_O_TRUNC;
    if( m & O_APPEND )
        res |= LFS_O_APPEND;

    return res;
}

int vfs_littlefs_open( void* ctx, const char* path, int flags, int /*mode*/ ) {
    assert( path );
    DECLARE_efs_CONTEXT_AND_HANDLE_LOCK( ctx );
    int fd;
    for( fd = 0; fd < efs->files.size(); ++fd ) {
        if( !efs->files[fd].is_used ) {
            int lfs_flags = littlefs_mode_conv( flags );
#if CONFIG_ESP_LITTLEFS_MALLOC
            efs->ret_code = lfs_file_open( &efs->fs, &efs->files[fd].file, path, lfs_flags );
#else
            efs->ret_code = lfs_file_opencfg( &efs->fs, &efs->files[fd].file, path, lfs_flags, &efs->files[fd].conf );
#endif
            CHECK_RET_CODE( efs->ret_code );
            efs->files[fd].is_used = true;
            return fd;
        }
    }
    return -1;
}

ssize_t vfs_littlefs_write( void* ctx, int fd, const void* data, size_t size ) {
    CHECK_FILE_DESCRIPTOR( fd );
    DECLARE_efs_CONTEXT_AND_HANDLE_LOCK( ctx );
    CHECK_FILE_IS_IN_USE( fd );

    efs->ret_code = lfs_file_write( &efs->fs, &efs->files[fd].file, (void*)data, size );
    CHECK_RET_CODE( efs->ret_code );
    return efs->ret_code;
}

ssize_t vfs_littlefs_read( void* ctx, int fd, void* dst, size_t size ) {
    CHECK_FILE_DESCRIPTOR( fd );
    DECLARE_efs_CONTEXT_AND_HANDLE_LOCK( ctx );
    CHECK_FILE_IS_IN_USE( fd );

    efs->ret_code = lfs_file_read( &efs->fs, &efs->files[fd].file, dst, size );
    CHECK_RET_CODE( efs->ret_code );
    return efs->ret_code;
}

int vfs_littlefs_close( void* ctx, int fd ) {
    CHECK_FILE_DESCRIPTOR( fd );
    DECLARE_efs_CONTEXT_AND_HANDLE_LOCK( ctx );
    CHECK_FILE_IS_IN_USE( fd );

    efs->ret_code = lfs_file_close( &efs->fs, &efs->files[fd].file );
    efs->files[fd].is_used = false;
    CHECK_RET_CODE( efs->ret_code );
    return 0;
}

int vfs_littlefs_fsync( void* ctx, int fd ) {
    CHECK_FILE_DESCRIPTOR( fd );
    DECLARE_efs_CONTEXT_AND_HANDLE_LOCK( ctx );
    CHECK_FILE_IS_IN_USE( fd );

    efs->ret_code = lfs_file_sync( &efs->fs, &efs->files[fd].file );
    CHECK_RET_CODE( efs->ret_code );
    return 0;
}

off_t vfs_littlefs_lseek( void* ctx, int fd, off_t offset, int mode ) {
    CHECK_FILE_DESCRIPTOR( fd );
    DECLARE_efs_CONTEXT_AND_HANDLE_LOCK( ctx );
    CHECK_FILE_IS_IN_USE( fd );

    efs->ret_code = lfs_file_seek( &efs->fs, &efs->files[fd].file, offset, mode );
    CHECK_RET_CODE( efs->ret_code );
    efs->ret_code = lfs_file_tell( &efs->fs, &efs->files[fd].file );
    CHECK_RET_CODE( efs->ret_code );
    return efs->ret_code;
}

int vfs_littlefs_fstat( void* ctx, int fd, struct stat* st ) {
    CHECK_FILE_DESCRIPTOR( fd );
    assert( st );
    DECLARE_efs_CONTEXT_AND_HANDLE_LOCK( ctx );
    CHECK_FILE_IS_IN_USE( fd );

    efs->ret_code = lfs_file_size( &efs->fs, &efs->files[fd].file );
    CHECK_RET_CODE( efs->ret_code );

    memset( st, 0, sizeof( *st ) );
    st->st_size = efs->ret_code;
    st->st_mode = S_IRWXU | S_IRWXG | S_IRWXO | S_IFREG;
    return 0;
}

int vfs_littlefs_stat( void* ctx, const char* path, struct stat* st ) {
    assert( path );
    assert( st );

    DECLARE_efs_CONTEXT_AND_HANDLE_LOCK( ctx );

    lfs_info info;
    efs->ret_code = lfs_stat( &efs->fs, path, &info );
    CHECK_RET_CODE( efs->ret_code );

    memset( st, 0, sizeof( *st ) );
    st->st_size = info.size;
    st->st_mode = S_IRWXU | S_IRWXG | S_IRWXO;
    st->st_mode |= ( info.type == LFS_TYPE_DIR ) ? S_IFDIR : S_IFREG;
    return 0;
}

int vfs_littlefs_rename( void* ctx, const char* src, const char* dst ) {
    assert( src );
    assert( dst );
    DECLARE_efs_CONTEXT_AND_HANDLE_LOCK( ctx );

    efs->ret_code = lfs_rename( &efs->fs, src, dst );
    CHECK_RET_CODE( efs->ret_code );
    return 0;
}

int vfs_littlefs_unlink( void* ctx, const char* path ) {
    assert( path );
    DECLARE_efs_CONTEXT_AND_HANDLE_LOCK( ctx );

    efs->ret_code = lfs_remove( &efs->fs, path );
    CHECK_RET_CODE( efs->ret_code );
    return 0;
}

DIR* vfs_littlefs_opendir( void* ctx, const char* name ) {
    assert( name );
    DECLARE_efs_CONTEXT_AND_HANDLE_LOCK( ctx );

    vfs_littlefs_dir_t* dir = new vfs_littlefs_dir_t;
    if( !dir ) {
        errno = ENOMEM;
        return NULL;
    }

    efs->ret_code = lfs_dir_open( &efs->fs, &dir->d, name );
    if( 0 > efs->ret_code ) {
        errno = littlefs_res_to_errno( efs->ret_code );
        delete dir;
        return NULL;
    }

    dir->path = name;

    return (DIR*)dir;
}

int vfs_littlefs_closedir( void* ctx, DIR* pdir ) {
    assert( pdir );
    DECLARE_efs_CONTEXT_AND_HANDLE_LOCK( ctx );

    vfs_littlefs_dir_t* dir = reinterpret_cast<vfs_littlefs_dir_t*>( pdir );

    efs->ret_code = lfs_dir_close( &efs->fs, &dir->d );
    delete dir;

    CHECK_RET_CODE( efs->ret_code );
    return 0;
}

struct dirent* vfs_littlefs_readdir( void* ctx, DIR* pdir ) {
    assert( pdir );

    struct dirent* out_dirent;
    vfs_littlefs_dir_t* dir = (vfs_littlefs_dir_t*)pdir;
    int err = vfs_littlefs_readdir_r( ctx, pdir, &dir->e, &out_dirent );
    if( err != 0 ) {
        errno = err;
        return NULL;
    }

    return out_dirent;
}

int vfs_littlefs_readdir_r( void* ctx, DIR* pdir, struct dirent* entry, struct dirent** out_dirent ) {
    assert( pdir );
    DECLARE_efs_CONTEXT_AND_HANDLE_LOCK( ctx );
    vfs_littlefs_dir_t* dir = (vfs_littlefs_dir_t*)pdir;

    lfs_info info;
    efs->ret_code = lfs_dir_read( &efs->fs, &dir->d, &info );
    if( 0 >= efs->ret_code ) {
        errno = littlefs_res_to_errno( efs->ret_code );
        *out_dirent = NULL;
        return efs->ret_code;
    }

    entry->d_ino = 0;
    entry->d_type = ( info.type == LFS_TYPE_DIR ) ? DT_DIR : DT_REG;
    snprintf( entry->d_name, sizeof( entry->d_name ), "%s", info.name );
    *out_dirent = entry;
    return 0;
}

long vfs_littlefs_telldir( void* ctx, DIR* pdir ) {
    assert( pdir );
    DECLARE_efs_CONTEXT_AND_HANDLE_LOCK( ctx );
    vfs_littlefs_dir_t* dir = (vfs_littlefs_dir_t*)pdir;

    efs->ret_code = lfs_dir_tell( &efs->fs, &dir->d );

    CHECK_RET_CODE( efs->ret_code );
    return efs->ret_code;
}

void vfs_littlefs_seekdir( void* ctx, DIR* pdir, long offset ) {
    assert( pdir );
    DECLARE_efs_CONTEXT_AND_HANDLE_LOCK( ctx );
    vfs_littlefs_dir_t* dir = (vfs_littlefs_dir_t*)pdir;

    efs->ret_code = lfs_dir_seek( &efs->fs, &dir->d, offset );
    if( 0 > efs->ret_code )
        errno = littlefs_res_to_errno( efs->ret_code );
}

int vfs_littlefs_mkdir( void* ctx, const char* name, mode_t mode ) {
    assert( name );
    DECLARE_efs_CONTEXT_AND_HANDLE_LOCK( ctx );

    efs->ret_code = lfs_mkdir( &efs->fs, name );
    CHECK_RET_CODE( efs->ret_code );

    // time_t t = time(NULL);
    // efs->ret_code = lfs_setattr(&efs->fs, name, ESL_LFS_MTIME, &t, sizeof(t));
    // CHECK_RET_CODE(efs->ret_code);

    return 0;
}

int vfs_littlefs_rmdir( void* ctx, const char* name ) {
    assert( name );
    DECLARE_efs_CONTEXT_AND_HANDLE_LOCK( ctx );

    efs->ret_code = lfs_remove( &efs->fs, name );
    CHECK_RET_CODE( efs->ret_code );
    return 0;
}

int vfs_littlefs_link( void* ctx, const char* n1, const char* n2 ) {
    errno = ENOTSUP;
    return -1;
}

/*int vfs_littlefs_utime(void* ctx, const char* path, const struct utimbuf* times)
{
    assert(path);
    assert(times);
    DECLARE_efs_CONTEXT_AND_HANDLE_LOCK(ctx);

    time_t t = time(NULL);
    efs->ret_code = lfs_setattr(&efs->fs, path, ESL_LFS_MTIME, &t, sizeof(t));
    CHECK_RET_CODE(efs->ret_code);

    return 0;
}*/

}  // namespace

extern "C" bool esp_is_littlefs_mounted( const char* partition_label ) {
    int index;
    if( esp_littlefs_by_label( partition_label, &index ) != ESP_OK ) {
        return false;
    }
    return _efs[index].is_mounted;
}

extern "C" esp_err_t esp_littlefs_info( const char* partition_label, size_t* total_bytes, size_t* used_bytes ) {
    int index;
    if( esp_littlefs_by_label( partition_label, &index ) != ESP_OK ) {
        return ESP_ERR_INVALID_STATE;
    }

    lfs_ssize_t sz = lfs_fs_size( &_efs[index].fs );
    if( 0 > sz )
        return -1;
    *total_bytes = _efs[index].cfg.block_size * _efs[index].cfg.block_count;
    *used_bytes = _efs[index].cfg.block_size * sz;
    return ESP_OK;
}

extern "C" esp_err_t esp_littlefs_format( const char* partition_label ) {
    bool partition_was_mounted = false;
    int index;
    esp_err_t err = esp_littlefs_by_label( partition_label, &index );
    if( err != ESP_OK ) {
        esp_vfs_littlefs_conf_t conf = {};
        conf.partition_label = partition_label;
        err = esp_littlefs_init( &conf, index );
        if( err != ESP_OK )
            return err;
    }
    else if( _efs[index].is_mounted ) {
        partition_was_mounted = true;
    }

    esp_littlefs_t& inst = _efs[index];
    lfs_t* fs = &inst.fs;

    inst.ret_code = lfs_unmount( fs );
    inst.is_mounted = false;
    if( 0 > inst.ret_code )
        return ESP_FAIL;

    inst.ret_code = lfs_format( fs, &inst.cfg );
    if( 0 > inst.ret_code )
        return ESP_FAIL;

    if( partition_was_mounted ) {
        inst.ret_code = lfs_mount( fs, &inst.cfg );
        if( 0 > inst.ret_code ) {
            ESP_LOGE( TAG, "mount failed, %i", inst.ret_code );
            return ESP_FAIL;
        }

        inst.is_mounted = true;
    }

    return ESP_OK;
}

extern "C" esp_err_t esp_vfs_littlefs_register( const esp_vfs_littlefs_conf_t* conf ) {
    assert( conf->base_path );
    esp_vfs_t vfs = {};
    vfs.flags = ESP_VFS_FLAG_CONTEXT_PTR;
    vfs.write_p = &vfs_littlefs_write;
    vfs.lseek_p = &vfs_littlefs_lseek;
    vfs.read_p = &vfs_littlefs_read;
    vfs.open_p = &vfs_littlefs_open;
    vfs.close_p = &vfs_littlefs_close;
    vfs.fsync_p = &vfs_littlefs_fsync;
    vfs.fstat_p = &vfs_littlefs_fstat;
    vfs.stat_p = &vfs_littlefs_stat;
    vfs.link_p = &vfs_littlefs_link;
    vfs.unlink_p = &vfs_littlefs_unlink;
    vfs.rename_p = &vfs_littlefs_rename;
    vfs.opendir_p = &vfs_littlefs_opendir;
    vfs.closedir_p = &vfs_littlefs_closedir;
    vfs.readdir_p = &vfs_littlefs_readdir;
    vfs.readdir_r_p = &vfs_littlefs_readdir_r;
    vfs.seekdir_p = &vfs_littlefs_seekdir;
    vfs.telldir_p = &vfs_littlefs_telldir;
    vfs.mkdir_p = &vfs_littlefs_mkdir;
    vfs.rmdir_p = &vfs_littlefs_rmdir;
    // vfs.utime_p     = &vfs_littlefs_utime;

    int index;
    esp_err_t err = esp_littlefs_init( conf, index );
    if( ESP_OK != err )
        return ESP_ERR_INVALID_STATE;

    ESP_LOGI( TAG, "begin register index %i", index );
    strlcpy( _efs[index].base_path, conf->base_path, sizeof( _efs[index].base_path ) );
    err = esp_vfs_register( _efs[index].base_path, &vfs, static_cast<void*>( _efs + index ) );
    if( ESP_OK == err ) {
        ESP_LOGI( TAG, "littlefs registered at index %i", index );
    }
    _efs[index].is_used = true;
    return err;
}

extern "C" esp_err_t esp_vfs_littlefs_unregister( const char* partition_label ) {
    int index;
    if( ESP_OK != esp_littlefs_by_label( partition_label, &index ) )
        return ESP_ERR_INVALID_STATE;

    esp_err_t err = esp_vfs_unregister( _efs[index].base_path );
    if( ESP_OK == err ) {
        _efs[index].is_used = false;
        ESP_LOGI( TAG, "littlefs unregistered at index %i", index );
    }
    return err;
}

extern "C" lfs_t* esp_littlefs_instance( const char* partition_label ) {
    int index;
    return ESP_OK == esp_littlefs_by_label( partition_label, &index ) ? &_efs[index].fs : nullptr;
}
