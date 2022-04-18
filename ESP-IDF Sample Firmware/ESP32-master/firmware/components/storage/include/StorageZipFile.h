#pragma once

#ifdef ZIP_ARCHIVE

#include <memory>
#include <string>

#include <StorageArchiveFile.h>
#include <miniz.h>

namespace storage {

class ZipFile : public ArchiveFile {
public:
    ZipFile();
    ~ZipFile();
    ZipFile( const ZipFile& ) = delete;
    ZipFile( ZipFile&& other );
    ZipFile& operator=( const ZipFile& ) = delete;
    ZipFile& operator=( ZipFile&& other );

    bool open( const char* zip_archive, const char* filename ) override;
    bool read( void* buff, size_t buff_size, size_t* output_size ) override;
    void close() override;
    size_t size() override;

private:
    void clear();

private:
    mz_zip_archive m_zip_archive;
    mz_zip_reader_extract_iter_state* m_file_iterator;
};

}  // namespace storage

#endif  // ZIP_ARCHIVE
