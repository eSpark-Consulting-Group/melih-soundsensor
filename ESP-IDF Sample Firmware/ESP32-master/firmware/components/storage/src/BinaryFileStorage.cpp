#include "BinaryFileStorage.h"
#include "BinaryFileStorageFactory.h"

#include <IReadStream.h>

#include <cstddef>
#include <sys/stat.h>
#include <unistd.h>
#include <cassert>
#include <cstdio>

//#define SIMPLE_KV_STOR_DEBUG
#ifdef SIMPLE_KV_STOR_DEBUG

#include <esp_log.h>
#include "EspLogTags.h"

#define SKVS_LOGI( ... ) ESP_LOGI(::esp_log_tags::BINARY_FILE_STORAGE, __VA_ARGS__ )
#define SKVS_LOGE( ... ) ESP_LOGE(::esp_log_tags::BINARY_FILE_STORAGE, __VA_ARGS__ )
#define SKVS_LOGW( ... ) ESP_LOGW(::esp_log_tags::BINARY_FILE_STORAGE, __VA_ARGS__ )

#else

#define SKVS_LOGI( ... ) \
    do {                 \
    } while( false )
#define SKVS_LOGE( ... ) \
    do {                 \
    } while( false )
#define SKVS_LOGW( ... ) \
    do {                 \
    } while( false )

#endif

namespace storage {

bool BinaryFileStorage::Entry::EntryHeader::write( file_t& _file ) {
    return sizeof( *this ) == ::write( _file, static_cast<void*>( this ), sizeof( *this ) );
}

void BinaryFileStorage::Entry::EntryHeader::reset() {
    total_len = 0;
    key_hash = 0;
    key_len = 0;
    data_type = NONE_TYPE;
    data_len = 0;
}

bool BinaryFileStorage::Entry::loadNext( const file_t& _file ) {
    m_offset += m_header.total_len;
    return m_offset == ::lseek( _file, m_offset, SEEK_SET ) &&
           sizeof( m_header ) == ::read( _file, static_cast<void*>( &m_header ), sizeof( m_header ) );
}

bool BinaryFileStorage::Entry::loadKey( const file_t& _file, std::string& _key ) const {
    _key.resize( m_header.key_len - 1, '\0' );
    // check that string really resized
    if( m_header.key_len - _key.length() != 1 )
        return false;
    int32_t offset = getKeyDataOffset();
    return offset == ::lseek( _file, offset, SEEK_SET ) &&
           ( m_header.key_len - 1 ) == read( _file, static_cast<void*>( &_key.front() ), ( m_header.key_len - 1 ) );
}

bool BinaryFileStorage::Entry::loadData( const file_t& _file, void* _buf, const uint16_t _len ) const {
    int32_t offset = getDataOffset();
    return offset == ::lseek( _file, offset, SEEK_SET ) && _len == ::read( _file, static_cast<void*>( _buf ), _len );
}

bool BinaryFileStorage::Entry::writeHeader( file_t& _file,
                                            const std::string& _key,
                                            const EntryHeader::DataType _type,
                                            const uint16_t data_len ) {
    if( m_offset != ::lseek( _file, m_offset, SEEK_SET ) )
        return false;

    m_header.key_len = _key.length() + 1;
    m_header.key_hash = calcCrc16( &_key.front(), _key.length(), 0 );
    m_header.data_type = _type;
    m_header.data_len = data_len;

    return m_header.write( _file ) && ( m_header.key_len == ::write( _file, _key.c_str(), m_header.key_len ) );
}

bool BinaryFileStorage::Entry::write( file_t& _file,
                                      const std::string& _key,
                                      const EntryHeader::DataType _type,
                                      const char* _buf,
                                      const uint16_t _data_len ) {
    return writeHeader( _file, _key, _type, _data_len ) && _data_len == ::write( _file, _buf, _data_len ) &&
           writeState( _file, EntryHeader::USED );
}

bool BinaryFileStorage::Entry::write( file_t& _file,
                                      const std::string& _key,
                                      const EntryHeader::DataType _type,
                                      common::IReadStream& _rstream,
                                      size_t _data_len ) {
    if( !writeHeader( _file, _key, _type, _data_len ) ) {
        return false;
    }

    char buf[128];
    size_t bytesToRead = std::min( sizeof( buf ), _data_len );
    while( bytesToRead ) {
        if( bytesToRead != _rstream.read( buf, bytesToRead ) || bytesToRead != ::write( _file, buf, bytesToRead ) )
            return false;

        _data_len -= bytesToRead;
        bytesToRead = std::min( sizeof( buf ), _data_len );
    }

    return writeState( _file, EntryHeader::USED );
}

bool BinaryFileStorage::Entry::readState( const file_t& _file, EntryHeader::States& _st ) const {
    return getStatesOffset() == ::lseek( _file, getStatesOffset(), SEEK_SET ) &&
           sizeof( _st ) == ::read( _file, static_cast<void*>( &_st ), sizeof( _st ) );
}

bool BinaryFileStorage::Entry::writeState( file_t& _file, EntryHeader::States _st ) {
    return getStatesOffset() == ::lseek( _file, getStatesOffset(), SEEK_SET ) &&
           sizeof( _st ) == ::write( _file, static_cast<void*>( &_st ), sizeof( _st ) );
}

uint16_t BinaryFileStorage::Entry::getDataSize( const EntryHeader::DataType _type ) {
    switch( _type ) {
    case EntryHeader::INT_8:
        return sizeof( std::int8_t );
    case EntryHeader::INT_16:
        return sizeof( std::int16_t );
    case EntryHeader::INT_32:
        return sizeof( std::int32_t );
    case EntryHeader::INT_64:
        return sizeof( std::int64_t );
    default:
        return 0;
    }
}

uint16_t BinaryFileStorage::Entry::calcSize( const std::string& _key, const uint16_t _buflen ) {
    return sizeof( EntryHeader ) + sizeof( EntryHeader::States ) + ( _key.length() + 1 ) + _buflen;
}

BinaryFileStorage::BinaryFileStorage( BinaryFileStorageFactory& _factory, const std::string& _namespace )
: m_factory( _factory )
, m_ref_counter( 1 )
, m_file( -1 )
, m_namespace( _namespace ) {
    assert( !_namespace.empty() );
    std::string fname = fullDbFilename();
    assert( !fname.empty() );

    if( !openDBFile( fname ) ) {
        SKVS_LOGI( "BinaryFileStorage: Create storage db file %s.\n", fname.c_str() );
        auto res = createDBFile( fname );
        assert( res );
    }

    if( !isDBFileValid() ) {
        SKVS_LOGW( "BinaryFileStorage: Invalid storage db file %s. Creating new db file instead of it.\n",
                   fname.c_str() );
        auto res = closeDBFile();
        assert( res );
        res &= createDBFile( fname );
        assert( res );
    }

    assert( 0 <= m_file );
}

BinaryFileStorage::~BinaryFileStorage() {
    closeDBFile();
    SKVS_LOGI( "BinaryFileStorage: Delete BinaryFileStorage for namespace: %s\n", m_namespace.c_str() );
}

bool BinaryFileStorage::loadValue( const std::string& _key,
                                   const Entry::EntryHeader::DataType _type,
                                   char* _buf,
                                   const uint16_t _data_len ) {
    assert( Entry::EntryHeader::TOTAL_TYPES_COUNT > _type );
    std::lock_guard<std::mutex> lock( m_mutex );
    Entry header = findEntry( _key, _type );
    bool is_successfull = !header.isEmpty() && loadData( header, _buf, _data_len );
    if( is_successfull )
        SKVS_LOGI( "BinaryFileStorage: Load data, type=%i, key=%s\n", (int)_type, _key.c_str() );
    return is_successfull;
}

bool BinaryFileStorage::isDBFileValid() {
    StorageHeader::GuardNum guard_num;
    return offsetof( StorageHeader, guard_num ) == ::lseek( m_file, offsetof( StorageHeader, guard_num ), SEEK_SET ) &&
           sizeof( guard_num ) == ::read( m_file, (void*)&guard_num, sizeof( guard_num ) ) &&
           guard_num == StorageHeader::GUARD_NUM;
}

std::string BinaryFileStorage::fullDbFilename() const {
    return std::string( BinaryFileStorageFactory::STORAGE_PATH ) + "/" + m_namespace;
}

bool BinaryFileStorage::openDBFile( const std::string& _filename ) {
    m_file = ::open( _filename.c_str(), O_RDWR, S_IRUSR | S_IWUSR );
    bool is_successfull = 0 <= m_file;

    if( is_successfull )
        SKVS_LOGI( "BinaryFileStorage: Open db file: %s\n", _filename.c_str() );

    return is_successfull;
}

bool BinaryFileStorage::createDBFile( const std::string& _filename ) {
    m_file = ::open( _filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR );
    if( 0 > m_file )
        return false;

    const StorageHeader::GuardNum guard_num = StorageHeader::GUARD_NUM;
    if( sizeof( guard_num ) != ::write( m_file, (void*)&guard_num, sizeof( guard_num ) ) )
        return false;

    int pos = ::lseek( m_file, 0, SEEK_CUR );
    if( 0 > pos || StorageHeader::TotalSize < pos )
        return false;

    const uint8_t zero = ( 0x0 );
    for( unsigned i = 0; i < unsigned( StorageHeader::TotalSize - pos ); ++i ) {
        size_t sz = ::write( m_file, (void*)&zero, 1 );
        if( 1 != sz )
            return false;
    }

    SKVS_LOGI( "BinaryFileStorage: Create db file: %s\n", _filename.c_str() );
    return true;
}

bool BinaryFileStorage::closeDBFile() {
    if( 0 > m_file )
        return false;
    file_t curr_file = m_file;
    m_file = -1;
    bool is_successfull = 0 == ::close( curr_file );

    if( is_successfull )
        SKVS_LOGI( "BinaryFileStorage: close db file for %s\n", m_namespace.c_str() );

    return is_successfull;
}

bool BinaryFileStorage::getInt8( const std::string& _key, std::int8_t& _value ) {
    return loadValue( _key, Entry::EntryHeader::INT_8, _value );
}

bool BinaryFileStorage::getInt16( const std::string& _key, std::int16_t& _value ) {
    return loadValue( _key, Entry::EntryHeader::INT_16, _value );
}

bool BinaryFileStorage::getInt32( const std::string& _key, std::int32_t& _value ) {
    return loadValue( _key, Entry::EntryHeader::INT_32, _value );
}

bool BinaryFileStorage::getInt64( const std::string& _key, std::int64_t& _value ) {
    return loadValue( _key, Entry::EntryHeader::INT_64, _value );
}

bool BinaryFileStorage::getBlob( const std::string& key, BlobType& value ) {
    std::lock_guard<std::mutex> lock( m_mutex );

    Entry header = findEntry( key, Entry::EntryHeader::BLOB );
    if( header.isEmpty() )
        return false;

    value.resize( header.m_header.data_len );
    if( value.size() != header.m_header.data_len ) {
        value.clear();
        return false;
    }

    bool is_successfull = loadData( header, static_cast<char*>( &value.front() ), header.m_header.data_len );

    if( is_successfull )
        SKVS_LOGI( "BinaryFileStorage: Load data, type=%i, key=%s\n", (int)Entry::EntryHeader::BLOB, key.c_str() );

    return is_successfull;
}

bool BinaryFileStorage::setInt8( const std::string& key, std::int8_t value ) {
    return saveData( key, Entry::EntryHeader::INT_8, value );
}

bool BinaryFileStorage::setInt16( const std::string& key, std::int16_t value ) {
    return saveData( key, Entry::EntryHeader::INT_16, value );
}

bool BinaryFileStorage::setInt32( const std::string& key, std::int32_t value ) {
    return saveData( key, Entry::EntryHeader::INT_32, value );
}

bool BinaryFileStorage::setInt64( const std::string& key, std::int64_t value ) {
    return saveData( key, Entry::EntryHeader::INT_64, value );
}

bool BinaryFileStorage::setBlob( const std::string& key, const BlobType& value ) {
    return saveData( key, Entry::EntryHeader::BLOB, &value.front(), value.size() );
}

bool BinaryFileStorage::setBlob( const std::string& key, common::IReadStream& rstream, const size_t len ) {
    return saveData( key, Entry::EntryHeader::BLOB, rstream, len );
}

bool BinaryFileStorage::exists( const std::string& key ) {
    std::lock_guard<std::mutex> lock( m_mutex );

    Entry header = findEntry( key, Entry::EntryHeader::ANY_TYPE );
    return !header.isEmpty();
}

bool BinaryFileStorage::remove( const std::string& key ) {
    std::lock_guard<std::mutex> lock( m_mutex );

    Entry header = findEntry( key, Entry::EntryHeader::ANY_TYPE );
    bool isSuccess = !header.isEmpty() && header.remove( m_file );
    if( isSuccess ) {
        ::fsync( m_file );
    }
    return isSuccess;
}

bool BinaryFileStorage::removeAll() {
    std::lock_guard<std::mutex> lock( m_mutex );
    std::string fname = fullDbFilename();
    return closeDBFile() && createDBFile( fname );
}

BinaryFileStorage::ConstIterator BinaryFileStorage::cbegin() const {
    struct SimpleIterator : KeyValueStorage::ConstIterator::IteratorImpl {
        using ValueType = KeyValueStorage::ConstIterator::ValueType;
        using EntryHeader = Entry::EntryHeader;

        Entry m_entry;
        const BinaryFileStorage& m_storage;

        SimpleIterator( const BinaryFileStorage& _stor )
        : m_storage( _stor ) {
            m_entry.beginIterate();
        }

        uptr clone() const override {
            return uptr( new SimpleIterator( *this ) );
        }

        bool next() override {
            if( m_entry.isEmpty() )
                return false;
            std::lock_guard<std::mutex> lock( m_storage.m_mutex );
            EntryHeader::States state;
            while( true ) {
                if( !m_entry.loadNext( m_storage.m_file ) || !m_entry.readState( m_storage.m_file, state ) ) {
                    // m_entry.reset();
                    return false;
                }
                if( Entry::EntryHeader::USED == state )
                    return true;
            }
        }

        uint32_t key_hash() override {
            return m_entry.m_header.key_hash;
        }

        const std::string key() const override {
            std::lock_guard<std::mutex> lock( m_storage.m_mutex );
            std::string key;
            if( 0 < m_entry.m_header.key_len )
                m_entry.loadKey( m_storage.m_file, key );
            return key;
        };

        const ValueType value() const override {
            std::lock_guard<std::mutex> lock( m_storage.m_mutex );
            ValueType val;
            switch( m_entry.m_header.data_type ) {
            case EntryHeader::INT_8: {
                std::int8_t val8;
                if( m_storage.loadData( m_entry, val8 ) )
                    val = val8;
                break;
            }
            case EntryHeader::INT_16: {
                std::int16_t val16;
                if( m_storage.loadData( m_entry, val16 ) )
                    val = val16;
                break;
            }
            case EntryHeader::INT_32: {
                std::int32_t val32;
                if( m_storage.loadData( m_entry, val32 ) )
                    val = val32;
                break;
            }
            case EntryHeader::INT_64: {
                std::int64_t val64;
                if( m_storage.loadData( m_entry, val64 ) )
                    val = val64;
                break;
            }
            case EntryHeader::BLOB: {
                BlobType blob;
                blob.resize( m_entry.m_header.data_len );
                if( blob.size() != m_entry.m_header.data_len ) {
                    blob.clear();
                    break;
                }

                if( m_storage.loadData( m_entry, static_cast<char*>( &blob.front() ), m_entry.m_header.data_len ) )
                    val = blob;
                break;
            }
            default:
                break;
            }

            return val;
        };
    };

    auto itr = std::make_unique<SimpleIterator>( *this );
    assert( itr );
    return itr->next() ? std::move( ConstIterator( std::move( itr ) ) ) : ConstIterator();
}

BinaryFileStorage::Entry BinaryFileStorage::findEntry( const std::string& _key, Entry::EntryHeader::DataType _type ) {
    const uint16_t key_hash = calcCrc16( &_key.front(), _key.length(), 0 );
    Entry _header;
    _header.beginIterate();
    Entry::EntryHeader::States state;
    std::string entry_key;
    while( _header.loadNext( m_file ) && _header.readState( m_file, state ) ) {
        if( state == Entry::EntryHeader::FREE )
            continue;

        if( Entry::EntryHeader::ANY_TYPE != _type && _header.m_header.data_type != _type )
            continue;

        if( 0 == _key.length() )
            return _header;

        if( key_hash != _header.m_header.key_hash )
            continue;

        if( !_header.loadKey( m_file, entry_key ) )
            break;

        if( _key == entry_key )
            return _header;
    }

    _header.reset();
    return _header;
}

BinaryFileStorage::Entry BinaryFileStorage::getEntryForWrite( const uint16_t _total_len ) {
    Entry _entry;
    _entry.beginIterate();
    Entry::EntryHeader::States state;
    while( _entry.loadNext( m_file ) && _entry.readState( m_file, state ) ) {
        if( Entry::EntryHeader::FREE == state && _total_len <= _entry.m_header.total_len ) {
            SKVS_LOGI( "BinaryFileStorage: Reuse entry, offset=%i, entry_len/data_len=%i/%i\n",
                       _entry.m_offset,
                       _entry.m_header.total_len,
                       _total_len );
            return _entry;
        }
    }

    // Got here if we reach the end of the file
    _entry.m_header.total_len = _total_len;
    return _entry;
}

bool BinaryFileStorage::saveData( const std::string& _key,
                                  const Entry::EntryHeader::DataType _type,
                                  const char* _buf,
                                  uint16_t _buflen ) {
    std::lock_guard<std::mutex> lock( m_mutex );

    Entry free_header = getEntryForWrite( Entry::calcSize( _key, _buflen ) );
    Entry old_header = findEntry( _key, Entry::EntryHeader::ANY_TYPE );

    if( !free_header.write( m_file, _key, _type, _buf, _buflen ) ) {
        return false;
    }

    bool is_successfull = old_header.isEmpty() || old_header.remove( m_file );
    if( is_successfull ) {
        ::fsync( m_file );
        SKVS_LOGI( "BinaryFileStorage: Save data(buffer), type=%i, key=%s\n", (int)_type, _key.c_str() );
    }
    return is_successfull;
}

bool BinaryFileStorage::saveData( const std::string& _key,
                                  const Entry::EntryHeader::DataType _type,
                                  common::IReadStream& _rstream,
                                  const size_t len ) {
    std::lock_guard<std::mutex> lock( m_mutex );

    Entry free_header = getEntryForWrite( Entry::calcSize( _key, len ) );
    Entry old_header = findEntry( _key, Entry::EntryHeader::ANY_TYPE );

    if( !free_header.write( m_file, _key, _type, _rstream, len ) ) {
        return false;
    }

    bool is_successfull = old_header.isEmpty() || old_header.remove( m_file );
    if( is_successfull ) {
        ::fsync( m_file );
        SKVS_LOGI( "BinaryFileStorage: Save data(stream), type=%i, key=%s\n", (int)_type, _key.c_str() );
    }
    return is_successfull;
}

}  // namespace storage
