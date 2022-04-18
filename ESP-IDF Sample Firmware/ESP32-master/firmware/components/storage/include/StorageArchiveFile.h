#pragma once

#include <cstdlib>
#include <memory>

namespace storage {

class ArchiveFile {
public:
    using UPtr = std::unique_ptr<ArchiveFile>;

    virtual ~ArchiveFile(){};
    virtual bool open( const char* zip_archive, const char* filename ) = 0;
    virtual bool read( void* buff, size_t buff_size, size_t* output_size ) = 0;
    virtual void close() = 0;
    virtual size_t size() = 0;

    virtual bool seek( size_t offset, int origin ) {
        return false;
    }
};

}  // namespace storage
