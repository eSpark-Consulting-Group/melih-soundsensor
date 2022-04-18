#pragma once

#include <chrono>
#include <memory>
#include <stdio.h>
#include <string>
#include <assert.h>
#include <znet/IFile.hpp>

namespace common {
class Buffer;

}  // namespace common

namespace storage {

/**
 * Interface for read/write operations with binary files.
 * Implementation is based on c standard file i/o API (stdio.h).
 * Should be avoided because of performance issue. Use BinaryFile class instead.
 */
class File : public IFile {
public:
    using UPtr = std::unique_ptr<File>;
    using Timeout = std::chrono::milliseconds;

public:
    File();
    ~File();

public:
    File( const File& ) = delete;
    File& operator=( const File& ) = delete;

public:
    bool open( const char* filename,
               const char* mode,
               Timeout timeout = Timeout( 100 ),
               bool enableTransaction = true );

    void close();

    FILE* get() {
        return m_file;
    }

    const std::string& getFilename() const;

    /**
     * @brief Access to write transaction flag
     *
     * Used only with file opened for write
     *
     * Example of use case:
     * @code
     * storage::File f;
     * bool& state = f.linkToTransactionStatus();
     * f.open( "<PATH>", "w" ); // <- set transaction flag to true
     * f.write(...);
     * state = fasle; // <- drop all changes in close();
     * f.close(); // <- check transaction flag (if (true) apply changes else drop changes
     * @endcode
     */
    bool& linkToTransactionStatus() {
        return m_hasTransactionSuccess;
    }

public:
    bool write( const void* src, size_t len ) override;

    bool read( void* dst, size_t len ) override;

    bool writeString( const char* value, uint16_t size );

    bool writeString( const char* value );

    inline bool write( const char* value ) {
        return writeString( value );
    };

    // write realSize and string content, and fill all not used bytes by end terminate symbol
    bool writeStringAdjustedBySize( const char* value, const uint16_t realSize, const uint16_t maxFieldSize );

    bool writeStringAdjustedBySize( const char* value, const uint16_t maxFieldSize );

    template <typename ValueType>
    inline bool write( const ValueType& value ) {
        return write( &value, sizeof( value ) );
    }

    template <typename ValueType>
    inline bool read( ValueType& value ) {
        return read( &value, sizeof( value ) ) == 1;
    }

    bool read( void* dst, size_t len, size_t* olen );

    bool readString( char* buffer, uint16_t maxSize );

    bool readString( std::string& buffer );

    bool readStringAdjustedBySize( char* buffer, const uint16_t maxFieldSize );

    int length();

public:
    bool skip( const uint32_t bytes );

    bool skipString();

    bool skipAdjustedString( const uint16_t maxFieldSize );

    bool eof();

    bool seek( size_t offset, int origin );

    size_t size() const;

private:
    bool hasWrittingOperationsFlags( const char* mode ) const;
    bool hasDropContentOperationsFlags( const char* mode ) const;

private:
    FILE* m_file = nullptr;
    std::string m_fileName;
    std::string m_fileNameTmp;
    bool m_hasWrittingOperation = false;
    bool m_hasTransactionSuccess = true;
    bool m_enableTransaction = true;
};

}  // namespace storage
