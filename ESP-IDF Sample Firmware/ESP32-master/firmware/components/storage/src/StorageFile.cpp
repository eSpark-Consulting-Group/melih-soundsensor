
#include <Storage.h>
#include <StorageFile.h>

#include <DebugUtils.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <memory>

namespace storage {

File::File() {}

File::~File() {
    close();
}

bool File::open( const char* filename, const char* mode, Timeout timeout, bool enableTransaction ) {
    assert( filename != nullptr || mode != nullptr );
    close();
    m_fileName = filename;
    m_hasTransactionSuccess = true;
    m_hasWrittingOperation = hasWrittingOperationsFlags( mode );
    m_enableTransaction = enableTransaction;
    if( m_hasWrittingOperation ) {
        if( m_enableTransaction ) {
            m_fileNameTmp = filename;
            m_fileNameTmp += "~";

            if( !hasDropContentOperationsFlags( mode ) ) {
                // copy content
                auto closeFile = []( FILE* f ) -> void {
                    if( f ) {
                        fflush( f );
                        fclose( f );
                    }
                };
                std::unique_ptr<FILE, void ( * )( FILE* )> fsrc( fopen( m_fileName.c_str(), "rb" ), closeFile );
                if( fsrc ) {  // if false then probably nothing to copy
                    std::unique_ptr<FILE, void ( * )( FILE* )> fdst( fopen( m_fileNameTmp.c_str(), "wb" ), closeFile );
                    if( !fdst ) {
                        FUNC_LOGW( "Cannot open file: %s(mode: %s). Error : %s.", filename, "wb", strerror( errno ) );
                        m_hasWrittingOperation = false;
                        return false;
                    }
                    char buff[512];
                    size_t size = 0;
                    while( ( size = fread( buff, 1, sizeof( buff ), fsrc.get() ) ) > 0 ) {
                        if( fwrite( buff, 1, size, fdst.get() ) != size ) {
                            FUNC_LOGW(
                                "Cannot open file: %s(mode: %s). Error : %s.", filename, mode, strerror( errno ) );
                            m_hasWrittingOperation = false;
                            return false;
                        }
                    }
                    if( ferror( fsrc.get() ) || ferror( fdst.get() ) ) {
                        FUNC_LOGW( "Cannot open file: %s(mode: %s). Error : %s.", filename, mode, strerror( errno ) );
                        m_hasWrittingOperation = false;
                        return false;
                    }
                    fflush( fdst.get() );
                }
            }  // Drop content
            filename = m_fileNameTmp.c_str();
        }
        else {  // Enable transaction
            m_fileNameTmp.clear();
        }
    }  // Has writing operation

    m_file = fopen( filename, mode );
    if( !m_file ) {
        FUNC_LOGW( "Cannot open file: %s(mode: %s). Error : %s.", filename, mode, strerror( errno ) );
        if( m_hasWrittingOperation ) {
            m_hasWrittingOperation = false;
        }
        return false;
    }

    return true;
}

void File::close() {
    if( m_file != nullptr ) {
        if( m_hasWrittingOperation ) {
            fflush( m_file );
        }

        fclose( m_file );
        m_file = nullptr;

        if( m_hasWrittingOperation ) {
            if( m_enableTransaction ) {
                if( m_hasTransactionSuccess ) {
                    unlink( m_fileName.c_str() );
                    rename( m_fileNameTmp.c_str(), m_fileName.c_str() );
                }
                else {
                    unlink( m_fileNameTmp.c_str() );
                }
            }
            /// INFO: do not clean m_hasTransactionSuccess here in this method (close)

            m_hasWrittingOperation = false;
        }
    }
    m_fileName.clear();
    m_fileNameTmp.clear();
}

const std::string& File::getFilename() const {
    return m_fileName;
};

bool File::hasWrittingOperationsFlags( const char* mode ) const {
    assert( mode );
    while( *mode != '\0' ) {
        if( ( *mode == 'w' ) || ( *mode == 'a' ) || ( *mode == '+' ) ) {
            return true;
        }

        ++mode;
    }

    return false;
}

bool File::hasDropContentOperationsFlags( const char* mode ) const {
    assert( mode );
    while( *mode != '\0' ) {
        if( *mode == 'w' ) {
            return true;
        }

        ++mode;
    }

    return false;
}

bool File::write( const void* src, size_t len ) {
    return fwrite( src, len, 1, m_file ) == 1;
}

bool File::writeString( const char* value, uint16_t size ) {
    assert( ( size && value ) || size == 0 );
    assert( m_file != nullptr && m_hasWrittingOperation );

    if( !value ) {
        size = 0;
    }

    return write( &size, sizeof( size ) ) && ( size == 0 || ( write( value, size ) ) );
}

bool File::writeString( const char* value ) {
    return writeString( value, value ? strlen( value ) : 0 );
}

bool File::writeStringAdjustedBySize( const char* value, const uint16_t realSize, const uint16_t maxFieldSize ) {
    if( realSize >= maxFieldSize ) {
        return false;
    }

    bool result = writeString( value, realSize );
    const auto notUsedBytes = maxFieldSize - realSize;

    for( size_t idx = 0; result && ( idx < notUsedBytes ); ++idx ) {
        static const auto zeroSymbol = '\0';
        result = fwrite( &zeroSymbol, 1, 1, m_file ) == 1;
    }

    return result;
}

bool File::writeStringAdjustedBySize( const char* value, const uint16_t maxFieldSize ) {
    return writeStringAdjustedBySize( value, value ? strlen( value ) : 0, maxFieldSize );
}

bool File::read( void* dst, size_t len ) {
    return len == 0 || fread( dst, len, 1, m_file ) == 1;
}

bool File::read( void* dst, size_t len, size_t* olen ) {
    size_t readLen = fread( dst, 1, len, m_file );
    if( readLen ) {
        *olen = readLen;
        return true;
    }
    return false;
}

bool File::readString( char* buffer, const uint16_t maxSize ) {
    uint16_t stringSize = 0;
    if( read( stringSize ) && ( stringSize < maxSize ) ) {
        if( fread( buffer, 1, stringSize, m_file ) == stringSize ) {
            buffer[stringSize] = '\0';
            return true;
        }
        else {
            buffer[0] = '\0';
        }
    }

    return false;
}

int File::length() {
    fseek( m_file, 0, SEEK_END );
    unsigned long len = (unsigned long)ftell( m_file );
    FUNC_LOGE( "size: %ld\n", len );
    fseek( m_file, 0L, SEEK_SET );

    return len;
}

bool File::readString( std::string& buffer ) {
    uint16_t stringSize = 0;
    if( read( stringSize ) ) {
        buffer.resize( stringSize + 1 );
        if( fread( const_cast<char*>( buffer.c_str() ), 1, stringSize, m_file ) == stringSize ) {
            buffer[stringSize] = '\0';
            return true;
        }
        else {
            buffer.clear();
        }
    }

    return false;
}

bool File::readStringAdjustedBySize( char* buffer, const uint16_t maxFieldSize ) {
    uint16_t stringSize = 0;
    if( read( stringSize ) && ( stringSize < maxFieldSize ) ) {
        if( fread( buffer, 1, stringSize, m_file ) == stringSize ) {
            buffer[stringSize] = '\0';
            return skip( maxFieldSize - stringSize );
        }
        else {
            buffer[0] = '\0';
        }
    }

    return false;
}

bool File::skip( const uint32_t bytes ) {
    assert( m_file != nullptr );
    return fseek( m_file, bytes, SEEK_CUR ) == 0;
}

bool File::skipString() {
    uint16_t stringSize = 0;
    return read( stringSize ) && skip( stringSize );
}

bool File::skipAdjustedString( const uint16_t maxFieldSize ) {
    return skip( sizeof( uint16_t ) + maxFieldSize );
}

bool File::eof() {
    assert( m_file != nullptr );
    return feof( m_file ) != 0;
}

bool File::seek( size_t offset, int origin ) {
    assert( m_file );
    return 0 == fseek( m_file, offset, origin );
}

size_t File::size() const {
    return std::numeric_limits<size_t>::max();
}

}  // namespace storage
