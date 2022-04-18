#pragma once

#include <StorageArchiveFile.h>
#include <StorageFileFormat.h>

namespace storage {

class ArchiveFileFactory {
public:
    static ArchiveFile::UPtr getArchiveFile( const char* archive_file );
    static ArchiveFile::UPtr getArchiveFile( FileFormat format );
    static FileFormat getFileFormatFromName( const char* file_name );
    static const char* getFileExtension( FileFormat format );
};

}  // namespace storage
