#include "FileSource.h"

namespace cloud_storage {

FileSource::FileSource( std::shared_ptr<std::ifstream> stream )
: m_stream( std::move( stream ) ) {}

size_t FileSource::read( size_t size, uint8_t* dst ) {
    return static_cast<std::size_t>(
        m_stream->read( reinterpret_cast<char*>( dst ), static_cast<std::streamsize>( size ) ).gcount() );
}

void FileSource::seek( size_t pos ) {
    m_stream->clear();  // reset fail flag
    m_stream->seekg( static_cast<std::streamsize>( pos ), std::ios::beg );
}

FileDestination::FileDestination( std::shared_ptr<std::ofstream> stream )
: m_stream( std::move( stream ) ) {}

void FileDestination::write( size_t size, const uint8_t* src ) {
    m_stream->write( reinterpret_cast<const char*>( src ), static_cast<std::streamsize>( size ) );
}

}  // namespace cloud_storage
