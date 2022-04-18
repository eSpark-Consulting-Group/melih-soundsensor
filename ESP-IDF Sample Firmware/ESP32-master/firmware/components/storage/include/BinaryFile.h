#pragma once

#include <optional>
#include <string>

namespace storage {

/**
 * Interface for read/write operations with binary files.
 * Implementation is based on POSIX file i/o API (sys/fcntl.h).
 * Should be preferred over StorageFile class.
 */
class BinaryFile {
public:
    static std::optional<BinaryFile> open( const std::string& path, bool enableWrite );

    BinaryFile( const BinaryFile& ) = delete;
    BinaryFile& operator=( const BinaryFile& ) = delete;

    BinaryFile( BinaryFile&& ) noexcept;
    BinaryFile& operator=( BinaryFile&& ) noexcept;

    ~BinaryFile() noexcept;

    int write( const void* data, size_t len );
    int read( void* buff, size_t len );

    bool seek( size_t offset );
    size_t seek_end();

private:
    using file_descriptor_t = int;

    explicit BinaryFile( file_descriptor_t );

    void close() noexcept;

    static bool isValidFileDescriptor( file_descriptor_t fd );
    static const file_descriptor_t INVALID_FILE_DESCRIPTOR;

private:
    file_descriptor_t m_fd = INVALID_FILE_DESCRIPTOR;
};

}  // namespace storage
