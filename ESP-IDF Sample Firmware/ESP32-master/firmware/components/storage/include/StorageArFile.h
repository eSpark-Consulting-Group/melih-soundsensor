#pragma once

#include <StorageFile.h>
#include <StorageArchiveFile.h>
#include <DebugUtils.h>

namespace common {

class Buffer;

}  // namespace common

namespace storage {

class ArFile : public ArchiveFile {
public:
    ArFile();
    ~ArFile();

public:
    ArFile( const ArFile& ) = delete;
    ArFile& operator=( const ArFile& ) = delete;

public:
    bool open( const char* archive, const char* filename ) override;

    void close() override;
    size_t size() override;

public:
    template <typename ValueType>
    bool read( ValueType& value ) {
        return read( (void*)&value, sizeof( value ) );
    }

    bool read( void* buffer, size_t size );

    bool read( void* buffer, size_t size, size_t* osize ) override;

    /*    bool readString( char* buffer, size_t maxSize );

        bool readString( common::Buffer& buffer );

        bool readStringAdjustedBySize( char* buffer, size_t maxSize );*/

public:
    bool skip( size_t size );

    /*    bool skipString();

        bool skipAdjustedString( size_t maxSize );*/

    bool eof();

private:
    File m_file;
    size_t m_offset = 0;
    size_t m_size = 0;
    size_t m_pos = 0;
};

}  // namespace storage
