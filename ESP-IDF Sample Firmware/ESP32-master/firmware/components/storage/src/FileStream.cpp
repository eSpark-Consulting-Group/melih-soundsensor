#include "FileStream.h"
#include "StorageArchiveFileFactory.h"

namespace storage {

WriteFileStream::UPtr WriteFileStream::open( const char* filename, const char* mode, bool enableTransaction ) {
    auto file = std::make_unique<File>();
    if( !file || !file->open( filename, mode, File::Timeout( 100 ), enableTransaction ) ) {
        return nullptr;
    }

    return UPtr( new WriteFileStream( std::move( file ) ) );
}

WriteFileStream::WriteFileStream( File::UPtr&& ptr )
: m_file( std::move( ptr ) ) {}

int WriteFileStream::write( const char* buffer, std::size_t size ) {
    return m_file && m_file->write( buffer, size ) ? size : 0;
}

bool WriteFileStream::flush() {
    return m_file && m_file->seek( 0, SEEK_SET );
}

bool WriteFileStream::close() {
    if( !m_file ) {
        return false;
    }
    m_file->close();
    return true;
}

ReadFileStream::UPtr ReadFileStream::open( const char* filename ) {
    auto file = std::make_unique<File>();
    if( !file || !file->open( filename, "r" ) ) {
        return nullptr;
    }
    auto stream = UPtr( new ReadFileStream( std::move( file ) ) );
    return stream;
}

ReadArchiveFileStream::UPtr ReadArchiveFileStream::open( const char* zip_archive, const char* filename ) {
    auto file = storage::ArchiveFileFactory::getArchiveFile( zip_archive );
    if( !file || !file->open( zip_archive, filename ) ) {
        return nullptr;
    }
    auto stream = UPtr( new ReadArchiveFileStream( std::move( file ) ) );
    return stream;
}

}  // namespace storage
