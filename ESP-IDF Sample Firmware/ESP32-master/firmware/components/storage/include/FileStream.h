#pragma once

#include "IReadStream.h"
#include "IWriteStream.h"
#include "StorageFile.h"
#include "StorageArchiveFile.h"

namespace storage {

class WriteFileStream : common::IWriteStream {
public:
    using UPtr = common::IWriteStream::UPtr;

    static UPtr open( const char* filename, const char* mode, bool enableTransaction = false );

    WriteFileStream() = default;

    WriteFileStream( File::UPtr&& ptr );

    int write( const char* buffer, std::size_t size ) override;

    bool flush() override;

    bool close() override;

private:
    File::UPtr m_file;
};

class ReadFileStream : public common::IReadStream {
public:
    using UPtr = std::unique_ptr<ReadFileStream>;

    static UPtr open( const char* filename );

    ReadFileStream() = default;

    ReadFileStream( File::UPtr&& ptr )
    : m_file( std::move( ptr ) ) {}

    int read( char* buff, size_t bufSize ) {
        size_t readed;
        return m_file && m_file->read( buff, bufSize, &readed ) ? readed : 0;
    }

    size_t size() const {
        return m_file ? m_file->size() : 0;
    }

    bool reset( unsigned int offset ) {
        return m_file ? m_file->seek( offset, SEEK_SET ) : false;
    }

    auto& file() {
        return m_file;
    }

private:
    File::UPtr m_file;
};

class ReadArchiveFileStream : public common::IReadStream {
public:
    using UPtr = std::unique_ptr<ReadArchiveFileStream>;

    static UPtr open( const char* zip_archive, const char* filename );

    ReadArchiveFileStream() = default;

    ReadArchiveFileStream( ArchiveFile::UPtr&& ptr )
    : m_file( std::move( ptr ) ) {}

    int read( char* buff, size_t bufSize ) {
        size_t readed;
        return m_file && m_file->read( buff, bufSize, &readed ) ? readed : 0;
    }

    size_t size() const {
        return m_file ? 1 : 0;
    }

    bool reset( unsigned int offset ) {
        return m_file ? m_file->seek( offset, SEEK_SET ) : false;
    }

    auto& file() {
        return m_file;
    }

private:
    ArchiveFile::UPtr m_file;
};

}  // namespace storage
