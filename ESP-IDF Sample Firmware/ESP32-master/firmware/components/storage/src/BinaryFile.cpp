#include "BinaryFile.h"

#include <cassert>
#include <fcntl.h>

namespace storage {

const BinaryFile::file_descriptor_t BinaryFile::INVALID_FILE_DESCRIPTOR = -1;

std::optional<BinaryFile> BinaryFile::open( const std::string& path, bool enableWrite ) {
    auto fd = ::open( path.c_str(), ( enableWrite ? O_RDWR | O_CREAT : O_RDONLY ), S_IRUSR | S_IWUSR );
    if( isValidFileDescriptor( fd ) ) {
        return BinaryFile{ fd };
    }
    return std::nullopt;
}

int BinaryFile::write( const void* data, size_t len ) {
    auto bytesWritten = ::write( m_fd, data, len );
    if( bytesWritten > 0 ) {
        ::fsync( m_fd );
    }
    return bytesWritten;
}

int BinaryFile::read( void* buff, size_t len ) {
    return ::read( m_fd, buff, len );
}

bool BinaryFile::seek( size_t offset ) {
    return offset == ::lseek( m_fd, offset, SEEK_SET );
}

size_t BinaryFile::seek_end() {
    return ::lseek( m_fd, 0, SEEK_END );
}

BinaryFile::BinaryFile( BinaryFile::file_descriptor_t fd )
: m_fd{ fd } {
    assert( isValidFileDescriptor( fd ) );
}

BinaryFile::BinaryFile( BinaryFile&& other ) noexcept
: m_fd( other.m_fd ) {
    other.m_fd = INVALID_FILE_DESCRIPTOR;
}

BinaryFile::~BinaryFile() noexcept {
    close();
}

BinaryFile& BinaryFile::operator=( BinaryFile&& other ) noexcept {
    close();
    std::swap( m_fd, other.m_fd );
    return *this;
}

void BinaryFile::close() noexcept {
    if( isValidFileDescriptor( m_fd ) ) {
        ::close( m_fd );
        m_fd = INVALID_FILE_DESCRIPTOR;
    }
}

bool BinaryFile::isValidFileDescriptor( file_descriptor_t fd ) {
    return fd >= 0;
}

}  // namespace storage
