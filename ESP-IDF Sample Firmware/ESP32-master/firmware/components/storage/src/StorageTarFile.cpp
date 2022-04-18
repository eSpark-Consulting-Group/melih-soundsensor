#include "StorageTarFile.h"
#include <cstring>
#include <set>
#include <cstdio>
#include <cstdint>
#include <climits>
#include <vector>
#include <DebugUtils.h>
#include <StorageFile.h>
#include <Volume.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>

#include <functional>
#include <algorithm>

#include "DebugUtils.h"

namespace storage {

namespace {

struct posix_header {   /* byte offset */
    char name[100];     /*   0 */
    char mode[8];       /* 100 */
    char uid[8];        /* 108 */
    char gid[8];        /* 116 */
    char size[12];      /* 124 */
    char mtime[12];     /* 136 */
    char chksum[8];     /* 148 */
    char typeflag;      /* 156 */
    char linkname[100]; /* 157 */
    char magic[6];      /* 257 */
    char version[2];    /* 263 */
    char uname[32];     /* 265 */
    char gname[32];     /* 297 */
    char devmajor[8];   /* 329 */
    char devminor[8];   /* 337 */
    char prefix[155];   /* 345 */
                        /* 500 */
};

constexpr char TMAGIC[] = "ustar"; /* ustar and a null */
constexpr auto TMAGLEN = 5u;
constexpr auto TAR_BLOCK_SIZE = 512u;
constexpr char GNU_LONGNAME_TYPE = 'L';
constexpr char INDEX_DIRECTORY[] = "/vol/taridx";

}  // namespace

bool TarFile::writeIndexRecords( const std::vector<IndexRecordKeyToValue>& files, const std::string& idx_filename ) {
    File idx_file;
    FUNC_LOGI( "Idx filename: %s", idx_filename.c_str() );
    checkIdxDirectory();
    if( !idx_file.open( idx_filename.c_str(), "wb" ) ) {
        FUNC_LOGE( "Can't create index file for %s archive", idx_filename.c_str() );
        return false;
    }
    auto result = idx_file.linkToTransactionStatus();

    result = result && common::FileIndexation::writeIndexFile( idx_file.get(), files );

    return result;
}

TarFile::TarFile() {
    init_positions();
}

TarFile::~TarFile() {
    close();
}

bool TarFile::open( const char* archive, const char* filename ) {
    if( !( archive && filename ) ) {
        return false;
    }

    size_t filename_length = strlen( filename );
    if( filename_length == 0 ) {
        return false;
    }

    std::vector<IndexRecordValue> foundRecords;

    if( !getPosFromIndex( archive, getIdxKey( filename ), foundRecords ) ) {
        FUNC_LOGW( "Failed to read position from index" );
        if( !createIndex( archive ) ) {
            FUNC_LOGE( "Failed to create idx for %s", archive );
            return false;
        }

        if( !getPosFromIndex( archive, getIdxKey( filename ), foundRecords ) ) {
            FUNC_LOGE( "Failed to read idx from archive %s", archive );
            return false;
        }
    }

    if( foundRecords.empty() ) {
#ifdef CONFIG_OPTIMIZATION_LEVEL_DEBUG
        FUNC_LOGI( "No file in archive %s", filename );
#endif
        return false;
    }

    if( !m_file.get() && !m_file.open( archive, "rb" ) ) {
        FUNC_LOGE( "Can't open tar archive file: %s", archive );
        return false;
    }

    std::vector<char> buffer( TAR_BLOCK_SIZE );
    std::vector<char> fileNamebuffer;

    const auto hasCollisions = foundRecords.size() > 1;

    for( const auto& posInfo : foundRecords ) {
        if( posInfo.longNameBlocks && hasCollisions ) {
            const auto bytesCount = TAR_BLOCK_SIZE * posInfo.longNameBlocks;
            fileNamebuffer.resize( bytesCount + 1 );

            if( !m_file.seek( posInfo.dataPos - bytesCount, SEEK_SET ) ) {
                FUNC_LOGE( "Failed to set position from idx" );
                return false;
            }

            m_file.read( buffer.data(), bytesCount );
            buffer[bytesCount] = '\0';
        }

        if( !m_file.seek( posInfo.dataPos, SEEK_SET ) ) {
            FUNC_LOGE( "Failed to set position from idx" );
            return false;
        }

        auto header = reinterpret_cast<posix_header*>( buffer.data() );
        size_t read_length{ 0u };
        m_file.read( header, TAR_BLOCK_SIZE, &read_length );

        if( read_length != TAR_BLOCK_SIZE || strncmp( TMAGIC, header->magic, TMAGLEN ) != 0 ) {
            return false;
        }

        if( hasCollisions ) {
            header->mode[0] =
                '\0';  // add string end to header: make following field mode broken ( it's not used actually )
            const char* currentFileName = posInfo.longNameBlocks ? fileNamebuffer.data() : header->name;
            if( strcmp( currentFileName, filename ) != 0 ) {
                continue;
            }
        }

        // Force 0 at the end of size field
        header->size[11] = 0;
        m_size = strtoul( header->size, nullptr, 8u );
        if( m_size == ULONG_MAX ) {
            return false;
        }

        const auto current_position = ftell( m_file.get() );
        if( current_position > 0 ) {
            m_offset = static_cast<decltype( m_offset )>( current_position );
            m_pos = m_offset;

            return true;
        }
        else {
            m_size = 0u;
            return false;
        }
    }

    FUNC_LOGE( "File infos found but nobody matches by filename %i %s", (int)foundRecords.size(), filename );

    return false;
}

bool TarFile::read( void* buff, size_t buff_size, size_t* output_size ) {
    if( !m_size ) {
        return false;
    }
    if( m_pos >= m_offset + m_size ) {
        return false;
    }
    buff_size = std::min( buff_size, m_offset + m_size - m_pos );
    if( m_file.read( buff, buff_size, output_size ) ) {
        m_pos += *output_size;
        return true;
    }
    return false;
}

void TarFile::close() {
    init_positions();
    m_file.close();
    m_indexFile.close();
    m_indexRecordsNumber = common::FileIndexation::InvalidRecordNumber;
}

size_t TarFile::size() {
    return m_size;
}

bool TarFile::seek( size_t offset, int origin ) {
    switch( origin ) {
    case SEEK_CUR:
        m_pos += offset;
        break;
    case SEEK_END:
        m_pos = m_offset + m_size - offset;
        break;
    case SEEK_SET:
        m_pos = m_offset + offset;
        break;
    default:
        return false;
    }
    return m_pos >= m_offset && m_pos < m_offset + m_size && m_file.seek( m_pos, SEEK_SET );
}

bool TarFile::createIndex( const char* archive ) {
    FUNC_LOGI( "start index creation  TarFile %s", archive );

    std::vector<IndexRecordKeyToValue> indexRecords;

    const auto result =
        forEachFileInArchive( archive, [&indexRecords]( const char* fileName, IndexRecordValue recordValue ) {
            indexRecords.emplace_back( IndexRecordKeyToValue{ getIdxKey( fileName ), recordValue } );
        } );

    if( !result )
        return false;

    indexRecords.shrink_to_fit();
    FUNC_LOGI( "Total %lu files found", static_cast<unsigned long>( indexRecords.size() ) );

    if( !writeIndexRecords( indexRecords, getIdxFilename( archive ) ) )
        return false;

    // Maybe return checkIndex( archive ); ??

    checkIndex( archive );

    return true;
}

bool TarFile::checkIndex( const char* archive ) {
    FUNC_LOGI( "start index checking %s", archive );

    File idxFile;
    if( !idxFile.open( getIdxFilename( archive ).c_str(), "rb" ) ) {
        FUNC_LOGE( "Can't open index" );
        return false;
    }

    unsigned long unexistingCount = 0;
    unsigned long foundFiles = 0;

    const auto recordsNumber = common::FileIndexation::getRecordsNumber<IndexRecordValue>( idxFile.get() );
    const auto result = forEachFileInArchive( archive, [&]( const char* fileName, IndexRecordValue recordValue ) {
        std::vector<IndexRecordValue> indexRecords;
        common::FileIndexation::findValuesInIndexFile(
            idxFile.get(), getIdxKey( fileName ), indexRecords, recordsNumber );

        const auto hasRecord =
            std::any_of( indexRecords.begin(), indexRecords.end(), [&recordValue]( const IndexRecordValue& r ) {
                return r.dataPos == recordValue.dataPos && r.longNameBlocks == recordValue.longNameBlocks;
            } );

        if( !hasRecord ) {
            FUNC_LOGE( "Can't find file %s", fileName );
            ++unexistingCount;
        }
        else {
            ++foundFiles;
        }
    } );

    FUNC_LOGI( "Index checking result. found: %lu; not found: %lu", foundFiles, unexistingCount );

    return result && unexistingCount == 0;
}

template <typename F>
bool TarFile::forEachFileInArchive( const char* archive, F&& f ) {
    if( !archive ) {
        return false;
    }
    File tar_file;
    if( !( tar_file.open( archive, "rb" ) && tar_file.seek( 0, SEEK_SET ) ) ) {
        FUNC_LOGE( "Can't open tar archive file: %s", archive );
        return false;
    }
    std::vector<char> buffer( TAR_BLOCK_SIZE );
    auto header = reinterpret_cast<posix_header*>( buffer.data() );

    size_t read_length = 0u;
    size_t file_size = 0u;
    IndexDataPos start_position = 0;

    char file_name[TAR_BLOCK_SIZE + 1] = { '\0' };

    while( start_position = ftell( tar_file.get() ), tar_file.read( header, TAR_BLOCK_SIZE, &read_length ) ) {
        if( read_length != TAR_BLOCK_SIZE ) {
            break;
        }
        if( strncmp( TMAGIC, header->magic, TMAGLEN ) != 0 ) {
            continue;
        }
        // Force 0 at the end of size field
        header->size[11] = 0;
        file_size = strtoul( header->size, nullptr, 8u );
        if( file_size == ULONG_MAX ) {
            continue;
        }
        if( header->typeflag == GNU_LONGNAME_TYPE ) {
            tar_file.read( file_name, TAR_BLOCK_SIZE, &read_length );
        }
        else {
            if( file_name[0] == '\0' ) {
                header->mode[0] =
                    '\0';  // add string end to header: make following field mode broken ( it's not used actually )
                f( header->name, IndexRecordValue{ 0, start_position } );
            }
            else {
                f( file_name, IndexRecordValue{ 1, start_position } );
                file_name[0] = '\0';
            }
            if( !tar_file.skip( ( file_size / TAR_BLOCK_SIZE + ( ( file_size % TAR_BLOCK_SIZE ) == 0 ? 0u : 1u ) ) *
                                TAR_BLOCK_SIZE ) ) {
                break;
            }
        }
    }
    tar_file.close();

    return true;
}

void TarFile::init_positions() {
    m_offset = 0u;
    m_size = 0u;
    m_pos = 0u;
}

TarFile::IndexKey TarFile::getIdxKey( const char* filename ) {
    return std::hash<std::string>{}( std::string( filename ) );
}

bool TarFile::getPosFromIndex( const char* archive, const IndexKey& key, std::vector<IndexRecordValue>& indexRecords ) {
    if( !m_indexFile.get() ) {
        if( !m_indexFile.open( getIdxFilename( archive ).c_str(), "rb" ) )
            return false;

        // cache records number because fseek to end of file takes some time
        m_indexRecordsNumber = common::FileIndexation::getRecordsNumber<IndexRecordValue>( m_indexFile.get() );
    }

    return common::FileIndexation::findValuesInIndexFile( m_indexFile.get(), key, indexRecords, m_indexRecordsNumber );
}

std::string TarFile::getIdxFilename( const char* archive ) {
    return std::string( INDEX_DIRECTORY )
        .append( "/" )
        .append( getHashedStr( archive ).append(
            common::FileIndexation::Revision ) );  // TODO: check correct index update on install via OTA and remove
                                                   // revision suffix
}

std::string TarFile::getHashedStr( const char* str ) {
    if( !str ) {
        str = "";
    }
    std::hash<std::string> hash_obj;
    const size_t hash = hash_obj( std::string( str ) );
    constexpr int name_length = sizeof( hash ) * 2 + 1;
    char name[name_length];
    name[name_length - 1] = 0;
    constexpr int mask = 0x0F;
    constexpr int shift = 0x61;
    for( int i = 0; i < sizeof( hash ); ++i ) {
        name[2 * i] = shift + ( hash >> ( 8 * i ) & mask );
        name[2 * i + 1] = shift + ( hash >> ( 8 * i + 4 ) & mask );
    }
    return { name };
}

void TarFile::checkIdxDirectory() {
    DIR* dir = opendir( INDEX_DIRECTORY );
    if( dir ) {
        auto res = closedir( dir );
        assert( 0 == res );
    }
    else {
        FUNC_LOGI( "Create %s directory", INDEX_DIRECTORY );
        assert( ENOENT == errno );
        auto res = mkdir( INDEX_DIRECTORY, 0 );
        assert( 0 == res );
    }
}

}  // namespace storage
