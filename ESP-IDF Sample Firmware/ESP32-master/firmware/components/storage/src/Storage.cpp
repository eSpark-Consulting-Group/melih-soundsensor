
#include <Storage.h>
#include <Volume.h>
#include <StoragePartitionLabel.h>

#include <esp_system.h>

#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include <algorithm>

#include <DebugUtils.h>

namespace storage {

Storage& Storage::get() {
    static Storage storage;
    return storage;
}

bool Storage::isExist( const char* filePath ) {
    if( !filePath ) {
        return false;
    }
    struct stat st;
    return stat( filePath, &st ) == 0;
}

bool Storage::remove( const char* filePath ) {
    if( !filePath ) {
        return false;
    }

    if( 0 == ::remove( filePath ) ) {
        return true;
    }

    DIR* dir = ::opendir( filePath );
    if( !dir ) {
        return false;
    }
    ::seekdir( dir, 2 );  // skip "." and ".."

    struct dirent* de = ::readdir( dir );
    while( de ) {
        if( de->d_type == DT_REG ) {
            // printf( "found file '%s'\n", de->d_name );
            char fname[256];
            auto writtenBytes = snprintf( fname, sizeof( fname ), "%s/%s", filePath, de->d_name );
            if( writtenBytes < 0 || writtenBytes == sizeof( fname ) ) {
                FUNC_LOGE( "Output has been truncated" );
                // TODO: implement correct problem handling
            }
            ::unlink( fname );
            // printf( "file: %s was removed \n", fname );
        }
        else if( de->d_type == DT_DIR ) {
            char subdir_name[256];
            auto writtenBytes = snprintf( subdir_name, sizeof( subdir_name ), "%s/%s", filePath, de->d_name );
            if( writtenBytes < 0 || writtenBytes == sizeof( subdir_name ) ) {
                FUNC_LOGE( "Output has been truncated" );
                // TODO: implement correct problem handling
            }
            remove( subdir_name );
        }
        else {
            FUNC_LOGW( "Unexpected entry type" );
        }

        de = ::readdir( dir );
    }

    return ( 0 == ::closedir( dir ) ) && ( 0 == ::rmdir( filePath ) );
}

bool Storage::rename( const char* oldFilePath, const char* newFilePath ) {
    if( !oldFilePath || !newFilePath ) {
        return false;
    }

    return ::rename( oldFilePath, newFilePath ) == 0;
}

bool Storage::walkDir( const char* dirPath, WalkDirClbk onFileClbk, WalkDirClbk onDirClbk ) {
    if( !dirPath || ( !onFileClbk && !onDirClbk ) ) {
        return false;
    }

    DIR* dir = ::opendir( dirPath );
    if( !dir ) {
        return false;
    }
    ::seekdir( dir, 2 );  // skip "." and ".."

    struct dirent* de = ::readdir( dir );
    while( de ) {
        if( de->d_type == DT_REG && onFileClbk ) {
            char fname[256];
            auto writtenBytes = snprintf( fname, sizeof( fname ), "%s/%s", dirPath, de->d_name );
            if( writtenBytes < 0 || writtenBytes == sizeof( fname ) ) {
                FUNC_LOGE( "Output has been truncated" );
                // TODO: implement correct problem handling
            }
            if( WalkDir::Complete == onFileClbk( de->d_name, fname ) ) {
                return true;
            }
        }
        else if( de->d_type == DT_DIR && onDirClbk ) {
            char subdir_name[256];
            auto writtenBytes = snprintf( subdir_name, sizeof( subdir_name ), "%s/%s", dirPath, de->d_name );
            if( writtenBytes < 0 || writtenBytes == sizeof( subdir_name ) ) {
                FUNC_LOGE( "Output has been truncated" );
                // TODO: implement correct problem handling
            }
            if( WalkDir::Complete == onDirClbk( de->d_name, subdir_name ) ) {
                return true;
            }
        }
        else {
            FUNC_LOGW( "Unexpected entry type" );
        }

        de = ::readdir( dir );
    }

    return 0 == ::closedir( dir );
}

bool Storage::createDir( const char* dirPath ) {
    if( !dirPath ) {
        return false;
    }

    DIR* dir = opendir( dirPath );
    if( dir ) {
        return 0 == closedir( dir );
    }
    return ENOENT == errno && 0 == mkdir( dirPath, 0 );
}

void Storage::printDir( const char* dirPath ) {
    if( !dirPath ) {
        printf( "\n------------Print directory: dir path is NULL\n" );
        return;
    }

    printf( "\n------------Print directory: %s/\n", dirPath );
    DIR* dir = ::opendir( dirPath );
    if( !dir ) {
        printf( "Can't open directory\n" );
        return;
    }
    ::seekdir( dir, 2 );  // skip "." and ".."

    struct dirent* de = ::readdir( dir );
    while( de ) {
        if( de->d_type == DT_REG ) {
            printf( "file: '%s'\n", de->d_name );
        }
        else if( de->d_type == DT_DIR ) {
            printf( "dir: '%s'\n", de->d_name );
        }
        else {
            FUNC_LOGW( "Unexpected entry type" );
        }

        de = ::readdir( dir );
    }

    ::closedir( dir );
    printf( "--------------------------------------\n" );
}

}  // namespace storage
