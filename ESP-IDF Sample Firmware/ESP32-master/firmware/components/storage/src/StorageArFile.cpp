/**
 * @brief Reader for files in AR file
 *
 * File format: https://en.wikipedia.org/wiki/Ar_(Unix)
 * Used System V (or GNU) variant
 */
#include <StorageArFile.h>

#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <vector>

#include <DebugUtils.h>

namespace storage {

namespace {

struct FILE_AR_HEADER {
    char name[16];
    char mtime[12];
    char uid[6];
    char gid[6];
    char mode[8];
    char size[10];
    char end[2];
};

}  // namespace

ArFile::ArFile() {}

ArFile::~ArFile() {
    close();
}

bool ArFile::open( const char* archive, const char* filename ) {
    assert( archive != nullptr || filename != nullptr );

    size_t filenameSize = strlen( filename );
    if( filenameSize == 0 ) {
        return false;
    }

    close();

    if( !m_file.open( archive, "rb" ) ) {
        return false;
    }

    char signature[8];
    if( !m_file.read( signature, sizeof( signature ) ) || strncmp( signature, "!<arch>\n", 8 ) ) {
        // not AR file
        close();
        return false;
    }
    FILE_AR_HEADER head;
    std::vector<char> filenamesInHeader;
    for( ;; ) {
        // Each archive file member begins on an even byte boundary
        long pos = ftell( m_file.get() );
        if( pos != EOF && pos & 0x01 ) {
            if( !m_file.skip( 1u ) ) {
                return false;
            }
        }
        memset( &head, 0, sizeof( FILE_AR_HEADER ) );
        if( !m_file.read( &head, sizeof( FILE_AR_HEADER ) ) || strncmp( head.end, "`\n", 2 ) ) {
            close();
            return false;
        }
        size_t fileSize = 0;
        if( sscanf( head.size, "%zu", &fileSize ) < 1 ) {
            close();
            return false;
        }
        bool found{ false };
        if( !strncmp( head.name, "//", 2 ) ) {
            filenamesInHeader.resize( fileSize, '\0' );
            if( !m_file.read( filenamesInHeader.data(), filenamesInHeader.size() ) ) {
                close();
                return false;
            }
            continue;
        }
        else {
            found = !strncmp( head.name, filename, std::min( filenameSize, (size_t)15 ) );
            if( !found && *head.name == '/' ) {
                size_t filenamePosition;
                if( sscanf( head.name, "/%zu", &filenamePosition ) == 1 ) {
                    found = filenamePosition < filenamesInHeader.size() &&
                            !strncmp( filenamesInHeader.data() + filenamePosition,
                                      filename,
                                      std::min( filenameSize, filenamesInHeader.size() - filenamePosition ) );
                }
            }
        }
        if( found ) {
            m_offset = ftell( m_file.get() );
            m_size = fileSize;
            m_pos = m_offset;
            break;
        }
        else {
            if( !m_file.skip( fileSize ) ) {
                close();
                return false;
            }
        }
    }

    return true;
}

void ArFile::close() {
    m_file.close();
    m_offset = 0;
    m_size = 0;
    m_pos = 0;
}

size_t ArFile::size() {
    return m_size;
}

bool ArFile::read( void* buffer, size_t size ) {
    if( !m_size ) {
        return false;
    }
    if( m_pos + size > m_offset + m_size ) {
        return false;
    }
    if( m_file.read( buffer, size ) ) {
        m_pos += size;
        return true;
    }
    return false;
}

bool ArFile::read( void* buffer, size_t size, size_t* osize ) {
    if( !m_size ) {
        return false;
    }
    if( m_pos >= m_offset + m_size ) {
        return false;
    }
    if( m_pos + size > m_offset + m_size ) {
        size = m_offset + m_size - m_pos;
    }
    if( m_file.read( buffer, size, osize ) ) {
        m_pos += *osize;
        return true;
    }
    return false;
}

/*bool ArFile::readString( char* buffer, size_t maxSize ) {
    uint16_t stringSize = 0;
    if( read( stringSize ) && ( stringSize < maxSize ) ) {
        if( read( buffer, stringSize ) ) {
            buffer[stringSize] = '\0';
            return true;
        }
        else {
            buffer[0] = '\0';
        }
    }

    return false;
}

bool ArFile::readString( common::Buffer& buffer ) {
    uint16_t stringSize = 0;
    if( read( stringSize ) && buffer.realloc( stringSize + 1 ) ) {
        if( read( buffer.get(), stringSize ) ) {
            buffer.get<char>()[stringSize] = '\0';
            return true;
        }
        else if( buffer ) {
            buffer.get<char>()[0] = '\0';
        }
    }

    return false;
}

bool ArFile::readStringAdjustedBySize( char* buffer, size_t maxSize ) {
    uint16_t stringSize = 0;
    if( read( stringSize ) && ( stringSize < maxSize ) ) {
        if( read( buffer, stringSize ) ) {
            buffer[stringSize] = '\0';
            return skip( maxSize - stringSize );
        }
        else {
            buffer[0] = '\0';
        }
    }

    return false;
}*/

bool ArFile::skip( size_t size ) {
    if( m_pos + size > m_offset + m_size ) {
        return false;
    }
    if( m_file.skip( size ) ) {
        m_pos += size;
        return true;
    }
    return false;
}

/*bool ArFile::skipString() {
    uint16_t stringSize = 0;
    return read( stringSize ) && skip( stringSize );
}

bool ArFile::skipAdjustedString( size_t maxSize ) {
    return skip( sizeof( uint16_t ) + maxSize );
}*/

bool ArFile::eof() {
    return m_file.eof() ? true : m_pos >= m_offset + m_size;
}

}  // namespace storage
