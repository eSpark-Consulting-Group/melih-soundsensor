#pragma once

#include <KeyValueStorage.h>
#include <cstdio>
#include <fcntl.h>
#include <mutex>
#include <crc16.h>

namespace storage {

class BinaryFileStorageFactory;

class BinaryFileStorage final : public KeyValueStorage {
    friend class BinaryFileStorageFactory;
    typedef int file_t;

#pragma pack( push, 1 )
    class StorageHeader {
    public:
        enum GuardNum : uint64_t { GUARD_NUM = 0xE715EE85EBF69D21 };
        enum { TotalSize = 256 };

        GuardNum guard_num;

    private:
        ~StorageHeader() = default;
    };
#pragma pack( pop )

    struct Entry {
#pragma pack( push, 1 )
        struct EntryHeader {
            enum States : uint8_t { FREE, USED };
            enum DataType : uint8_t { INT_8, INT_16, INT_32, INT_64, BLOB, TOTAL_TYPES_COUNT, NONE_TYPE, ANY_TYPE };

            uint16_t total_len;
            uint16_t key_hash;
            uint16_t key_len;
            DataType data_type;
            uint16_t data_len;

            bool write( file_t& _file );

            void reset();
        };
#pragma pack( pop )

        static uint16_t getDataSize( const EntryHeader::DataType _type );

        static uint16_t calcSize( const std::string& _key, const uint16_t _buflen );

        int32_t m_offset;
        EntryHeader m_header;

        inline int32_t getStatesOffset() const {
            return m_offset + m_header.total_len - sizeof( EntryHeader::States );
        }

        inline int32_t getKeyDataOffset() const {
            return m_offset + sizeof( m_header );
        }

        inline int32_t getDataOffset() const {
            return getKeyDataOffset() + m_header.key_len;
        }

        inline void beginIterate() {
            m_offset = StorageHeader::TotalSize;
            m_header.total_len = 0;
        }

        inline void reset() {
            m_offset = -1;
            m_header.reset();
        }

        inline bool isEmpty() const {
            return 0 > m_offset;
        }

        inline bool remove( file_t& _file ) {
            return writeState( _file, EntryHeader::FREE );
        }

        Entry()
        : m_offset( -1 ) {}

        bool loadNext( const file_t& _file );

        bool loadKey( const file_t& _file, std::string& _key ) const;

        bool loadData( const file_t& _file, void* _buf, const uint16_t _len ) const;

        bool writeHeader( file_t& _file,
                          const std::string& _key,
                          const EntryHeader::DataType _type,
                          const uint16_t data_len );

        bool write( file_t& _file,
                    const std::string& _key,
                    const EntryHeader::DataType _type,
                    const char* _buf,
                    const uint16_t _data_len );

        bool write( file_t& _file,
                    const std::string& _key,
                    const EntryHeader::DataType _type,
                    common::IReadStream& rstream,
                    size_t _data_len );

        bool readState( const file_t& _file, EntryHeader::States& _st ) const;

        bool writeState( file_t& _file, EntryHeader::States _st );
    };  // end of Entry

    BinaryFileStorage( BinaryFileStorageFactory& _factory, const std::string& _namespace );

    BinaryFileStorageFactory& m_factory;
    unsigned int m_ref_counter;
    file_t m_file;
    const std::string m_namespace;
    mutable std::mutex m_mutex;

    ~BinaryFileStorage();

public:
    bool getInt8( const std::string& key, std::int8_t& value ) override;
    bool getInt16( const std::string& key, std::int16_t& value ) override;
    bool getInt32( const std::string& key, std::int32_t& value ) override;
    bool getInt64( const std::string& key, std::int64_t& value ) override;
    bool getBlob( const std::string& key, BlobType& value ) override;
    bool setInt8( const std::string& key, std::int8_t value ) override;
    bool setInt16( const std::string& key, std::int16_t value ) override;
    bool setInt32( const std::string& key, std::int32_t value ) override;
    bool setInt64( const std::string& key, std::int64_t value ) override;
    bool setBlob( const std::string& key, const BlobType& value ) override;
    bool setBlob( const std::string& key, common::IReadStream& rstream, const size_t len ) override;
    bool exists( const std::string& key ) override;
    bool remove( const std::string& key ) override;
    bool removeAll() override;

    ConstIterator cbegin() const override;

    bool isDBFileValid();

private:
    Entry findEntry( const std::string& _key, Entry::EntryHeader::DataType _type );

    Entry getEntryForWrite( const uint16_t _total_len );

    bool loadValue( const std::string& _key,
                    const Entry::EntryHeader::DataType _type,
                    char* _buf,
                    const uint16_t _data_len );

    template <typename T>
    inline bool loadValue( const std::string& _key, const Entry::EntryHeader::DataType _type, T& _val ) {
        return loadValue( _key, _type, reinterpret_cast<char*>( &_val ), sizeof( _val ) );
    }

    inline bool loadData( const Entry& _entry, char* _buf, const uint16_t _len ) const {
        return _entry.loadData( m_file, _buf, _len );
    }

    template <typename T>
    inline bool loadData( const Entry& _entry, T& _val ) const {
        return loadData( _entry, reinterpret_cast<char*>( &_val ), sizeof( _val ) );
    }

    bool saveData( const std::string& _key,
                   const Entry::EntryHeader::DataType _type,
                   const char* _buf,
                   uint16_t _buflen );

    template <typename T>
    bool saveData( const std::string& _key, const Entry::EntryHeader::DataType _type, T& _val ) {
        return saveData( _key, _type, reinterpret_cast<const char*>( &_val ), sizeof( _val ) );
    }

    bool saveData( const std::string& _key,
                   const Entry::EntryHeader::DataType _type,
                   common::IReadStream& _rstream,
                   const size_t len );

    std::string fullDbFilename() const;

    bool openDBFile( const std::string& _filename );

    bool createDBFile( const std::string& _filename );

    bool closeDBFile();
};

}  // end of namespace storage
