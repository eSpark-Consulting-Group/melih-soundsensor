idf_component_register(SRCS "littlefs-master/lfs.c" "littlefs-master/lfs_util.c"
                       INCLUDE_DIRS "littlefs-master"
                       REQUIRES esp32 soc)

set_source_files_properties(littlefs-master/lfs.c
    PROPERTIES COMPILE_DEFINITIONS LFS_CONFIG=../littlefs_config.h
)
set_source_files_properties(littlefs-master/lfs_util.c
    PROPERTIES COMPILE_DEFINITIONS LFS_CONFIG=../littlefs_config.h
)
include(../config_flags_throw.cmake)