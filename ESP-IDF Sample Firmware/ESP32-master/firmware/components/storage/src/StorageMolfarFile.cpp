#include "StorageMolfarFile.h"
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

struct molfar_header {
    uint32_t filePathSize;
    uint32_t contentSize;
};

constexpr char INDEX_DIRECTORY[] = "/vol/taridx";

}  // namespace

uint32_t getFullBlockSize( uint32_t size ) {
    return ( size + 3u ) & ~3u;
}

bool MolfarFile::writeIndexRecords( const std::vector<IndexRecordKeyToValue>& files, const std::string& idx_filename ) {
    File idx_file;
    checkIdxDirectory();
    if( !idx_file.open( idx_filename.c_str(), "wb" ) ) {
        FUNC_LOGE( "Can't create index file for %s archive", idx_filename.c_str() );
        return false;
    }
    auto result = idx_file.linkToTransactionStatus();

    result = result && common::FileIndexation::writeIndexFile( idx_file.get(), files );

    return result;
}

MolfarFile::MolfarFile() {
    init_positions();
}

MolfarFile::~MolfarFile() {
    close();
}

bool MolfarFile::open( const char* archive, const char* filename ) {
    if( !( archive && filename ) ) {
        return false;
    }

    size_t filename_length = strlen( filename );
    if( filename_length == 0 ) {
        return false;
    }

    std::vector<IndexRecordValue> foundRecords;

    if( !getPosFromIndex( archive, getIdxKey( filename ), foundRecords ) ) {
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
        FUNC_LOGE( "Can't open molfar archive file: %s", archive );
        return false;
    }

    std::vector<char> buffer( sizeof( molfar_header ) );
    std::vector<char> fileNamebuffer;

    const auto hasCollisions = foundRecords.size() > 1;

    for( const auto& posInfo : foundRecords ) {
        if( !m_file.seek( posInfo.dataPos, SEEK_SET ) ) {
            FUNC_LOGE( "Failed to set position from idx" );
            return false;
        }

        auto header = reinterpret_cast<molfar_header*>( buffer.data() );
        size_t read_length{ 0u };
        m_file.read( header, sizeof( molfar_header ), &read_length );

        if( read_length != sizeof( molfar_header ) ) {
            return false;
        }

        m_size = header->contentSize;
        if( m_size == ULONG_MAX ) {
            return false;
        }

        if( hasCollisions ) {
            fileNamebuffer.resize( header->filePathSize + 1 );
            m_file.read( fileNamebuffer.data(), header->filePathSize, &read_length );
            if( read_length != header->filePathSize ) {
                return false;
            }
            fileNamebuffer.push_back( '\0' );

            if( strcmp( fileNamebuffer.data(), filename ) != 0 ) {
                continue;
            }
        }

        auto current_position = ftell( m_file.get() );
        current_position += getFullBlockSize( header->filePathSize );
        m_file.seek( current_position, SEEK_SET );

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

    return false;
}

bool MolfarFile::read( void* buff, size_t buff_size, size_t* output_size ) {
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

void MolfarFile::close() {
    init_positions();
    m_file.close();
    m_indexFile.close();
    m_indexRecordsNumber = common::FileIndexation::InvalidRecordNumber;
}

size_t MolfarFile::size() {
    return m_size;
}

bool MolfarFile::seek( size_t offset, int origin ) {
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

bool MolfarFile::createIndex( const char* archive ) {
    std::vector<IndexRecordKeyToValue> indexRecords;
    const auto result =
        forEachFileInArchive( archive, [&indexRecords]( const char* fileName, IndexRecordValue recordValue ) {
            indexRecords.emplace_back( IndexRecordKeyToValue{ getIdxKey( fileName ), recordValue } );
        } );

    if( !result )
        return false;

    indexRecords.shrink_to_fit();

    if( indexRecords.size() > 0 && !writeIndexRecords( indexRecords, getIdxFilename( archive ) ) )
        return false;

    // Maybe return checkIndex( archive ); ??

    checkIndex( archive );

    return true;
}

bool MolfarFile::checkIndex( const char* archive ) {
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

    return result && unexistingCount == 0;
}

template <typename F>
bool MolfarFile::forEachFileInArchive( const char* archive, F&& f ) {
    if( !archive ) {
        return false;
    }

    File molfar_file;
    if( !( molfar_file.open( archive, "rb" ) && molfar_file.seek( 0, SEEK_SET ) ) ) {
        FUNC_LOGE( "Can't open molfar archive file: %s", archive );
        return false;
    }
    std::vector<char> buffer( sizeof( molfar_header ) );
    auto header = reinterpret_cast<molfar_header*>( buffer.data() );

    size_t read_length = 0u;
    IndexDataPos start_position = 0;

    std::vector<char> file_name;
    // char file_name[TAR_BLOCK_SIZE + 1] = { '\0' };

    while( start_position = ftell( molfar_file.get() ),
           molfar_file.read( header, sizeof( molfar_header ), &read_length ) ) {
        if( read_length != sizeof( molfar_header ) ) {
            break;
        }

        file_name.resize( header->filePathSize + 1 );
        molfar_file.read( file_name.data(), header->filePathSize, &read_length );

        if( read_length != header->filePathSize ) {
            return false;
        }
        file_name.data()[header->filePathSize] = ( '\0' );

        f( file_name.data(), IndexRecordValue{ 0, start_position } );

        if( !molfar_file.skip( getFullBlockSize( header->filePathSize ) - header->filePathSize +
                               getFullBlockSize( header->contentSize ) ) ) {
            break;
        }
    }
    molfar_file.close();

    return true;
}

void MolfarFile::init_positions() {
    m_offset = 0u;
    m_size = 0u;
    m_pos = 0u;
}

MolfarFile::IndexKey MolfarFile::getIdxKey( const char* filename ) {
    return std::hash<std::string>{}( std::string( filename ) );
}

bool MolfarFile::getPosFromIndex( const char* archive,
                                  const IndexKey& key,
                                  std::vector<IndexRecordValue>& indexRecords ) {
    if( !m_indexFile.get() ) {
        if( !m_indexFile.open( getIdxFilename( archive ).c_str(), "rb" ) )
            return false;

        // cache records number because fseek to end of file takes some time
        m_indexRecordsNumber = common::FileIndexation::getRecordsNumber<IndexRecordValue>( m_indexFile.get() );
    }

    return common::FileIndexation::findValuesInIndexFile( m_indexFile.get(), key, indexRecords, m_indexRecordsNumber );
}

std::string MolfarFile::getIdxFilename( const char* archive ) {
    return std::string( INDEX_DIRECTORY )
        .append( "/" )
        .append( getHashedStr( archive ).append(
            common::FileIndexation::Revision ) );  // TODO: check correct index update on install via OTA and remove
                                                   // revision suffix
}

std::string MolfarFile::getHashedStr( const char* str ) {
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

void MolfarFile::checkIdxDirectory() {
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
