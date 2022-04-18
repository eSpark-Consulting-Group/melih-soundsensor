#pragma once

#include <StorageArchiveFile.h>
#include <cstddef>
#include <StorageFile.h>

#include "FileIndexation.h"

namespace storage {

class TarFile : public ArchiveFile {
public:
    TarFile();
    ~TarFile();
    TarFile( const TarFile& ) = delete;
    TarFile( TarFile&& other ) = delete;
    TarFile& operator=( const TarFile& ) = delete;
    TarFile& operator=( TarFile&& other ) = delete;

    bool open( const char* archive, const char* filename ) override;
    bool read( void* buff, size_t buff_size, size_t* output_size ) override;
    void close() override;
    size_t size() override;

    bool seek( size_t offset, int origin ) override;

    static bool createIndex( const char* archive );

private:
    using IndexKey = common::FileIndexation::HashType;
    using IndexDataPos = uint32_t;

    struct __attribute__( ( packed ) ) IndexRecordValue {
        uint8_t longNameBlocks = 0;
        IndexDataPos dataPos = std::numeric_limits<IndexDataPos>::max();
    };

    using IndexRecord = common::FileIndexation::IndexRecord<IndexRecordValue>;
    using IndexRecordKeyToValue = common::FileIndexation::HashToValue<IndexRecordValue>;

    void init_positions();
    static std::string getIdxFilename( const char* archive );
    static IndexKey getIdxKey( const char* filename );
    bool getPosFromIndex( const char* archive, const IndexKey& key, std::vector<IndexRecordValue>& filePosInfo );
    static std::string getHashedStr( const char* str );
    static void checkIdxDirectory();

    static bool writeIndexRecords( const std::vector<IndexRecordKeyToValue>& files, const std::string& idx_filename );

    template <typename F>
    static bool forEachFileInArchive( const char* archive, F&& f );

    static bool checkIndex( const char* archive );

private:
    File m_indexFile;
    common::FileIndexation::IndexRecordNumber m_indexRecordsNumber = common::FileIndexation::InvalidRecordNumber;
    File m_file;
    size_t m_offset;
    size_t m_size;
    size_t m_pos;
};

}  // namespace storage
