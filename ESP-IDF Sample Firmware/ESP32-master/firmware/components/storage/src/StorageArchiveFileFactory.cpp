#include "StorageArchiveFileFactory.h"

#include <utility>
#include "Utils.h"
#include "StorageArFile.h"
#ifdef ZIP_ARCHIVE
#include "StorageZipFile.h"
#endif

#ifdef TAR_FORMAT
#include "StorageTarFile.h"
#endif

#ifdef MOLFAR_FORMAT
#include "StorageMolfarFile.h"
#endif

namespace storage {

ArchiveFile::UPtr ArchiveFileFactory::getArchiveFile( const char* archive_file ) {
    return getArchiveFile( getFileFormatFromName( archive_file ) );
}

ArchiveFile::UPtr ArchiveFileFactory::getArchiveFile( FileFormat format ) {
    std::unique_ptr<ArchiveFile> file;
    switch( format ) {
    case FileFormat::UNKNOWN:
        break;
    case FileFormat::AR:
        file = std::make_unique<ArFile>();
        break;
#ifdef ZIP_ARCHIVE
    case FileFormat::ZIP:
        file = std::make_unique<ZipFile>();
        break;
#endif
#ifdef TAR_FORMAT
    case FileFormat::TAR_ZZ:
        break;
#endif
#ifdef MOLFAR_FORMAT
    case FileFormat::MOLFAR_ZZ:
        break;
#endif
#ifdef TAR_FORMAT
    case FileFormat::TAR:
        file = std::make_unique<TarFile>();
        break;
#endif
#ifdef MOLFAR_FORMAT
    case FileFormat::MOLFAR:
        file = std::make_unique<MolfarFile>();
        break;
#endif
    }
    return file;
}

FileFormat ArchiveFileFactory::getFileFormatFromName( const char* file_name ) {
    FileFormat file_format = FileFormat::UNKNOWN;
    if( common::stringEndsWith( file_name, AR_EXTENSION ) ) {
        file_format = FileFormat::AR;
    }
#ifdef TAR_FORMAT
    else if( common::stringEndsWith( file_name, TAR_EXTENSION ) ) {
        file_format = FileFormat::TAR;
    }
#endif
#ifdef MOLFAR_FORMAT
    else if( common::stringEndsWith( file_name, MOLFAR_EXTENSION ) ) {
        file_format = FileFormat::MOLFAR;
    }
#endif
#ifdef ZIP_ARCHIVE
    else if( common::stringEndsWith( file_name, ZIP_EXTENSION ) ) {
        file_format = FileFormat::ZIP;
    }
#endif
#ifdef TAR_FORMAT
    else if( common::stringEndsWith( file_name, TAR_ZZ_EXTENSION ) ) {
        file_format = FileFormat::TAR_ZZ;
    }
#endif
#ifdef MOLFAR_FORMAT
    else if( common::stringEndsWith( file_name, MOLFAR_ZZ_EXTENSION ) ) {
        file_format = FileFormat::MOLFAR_ZZ;
    }
#endif

    return file_format;
}

const char* ArchiveFileFactory::getFileExtension( FileFormat format ) {
    const char* extension{ nullptr };
    switch( format ) {
    case FileFormat::UNKNOWN:
        extension = UNKNOWN_EXTENSION;
        break;
    case FileFormat::AR:
        extension = AR_EXTENSION;
        break;
#ifdef ZIP_ARCHIVE
    case FileFormat::ZIP:
        extension = ZIP_EXTENSION;
        break;
#endif
#ifdef TAR_FORMAT
    case FileFormat::TAR_ZZ:
        extension = TAR_ZZ_EXTENSION;
        break;
#endif
#ifdef MOLFAR_FORMAT
    case FileFormat::MOLFAR_ZZ:
        extension = MOLFAR_ZZ_EXTENSION;
        break;
#endif
#ifdef TAR_FORMAT
    case FileFormat::TAR:
        extension = TAR_EXTENSION;
        break;
#endif
#ifdef MOLFAR_FORMAT
    case FileFormat::MOLFAR:
        extension = MOLFAR_EXTENSION;
        break;
#endif
    }
    return extension;
}

}  // namespace storage
