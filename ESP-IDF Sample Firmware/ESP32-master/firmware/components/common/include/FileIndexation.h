#pragma once

#include <cstddef>
#include <cstdint>

#include <limits>
#include <utility>
#include <vector>
#include <cstdio>
#include <cassert>

#include "DebugUtils.h"

namespace common {

namespace FileIndexation {

constexpr const char Revision[] = "1.0";

using IndexRecordNumber = uint16_t;
using HashType = size_t;
constexpr IndexRecordNumber InvalidRecordNumber = std::numeric_limits<IndexRecordNumber>::max();

struct __attribute__( ( packed ) ) RecordInfo {
    IndexRecordNumber hashToValueIndex = InvalidRecordNumber;
    IndexRecordNumber nextRecordNumber = InvalidRecordNumber;
    uint8_t number = 0;
};

template <typename Value>
struct __attribute__( ( packed ) ) IndexRecord {
    HashType hash = 0;
    IndexRecordNumber nextRecordNumber = InvalidRecordNumber;
    uint8_t number = 0;
    Value value{};
};

template <typename Value>
struct __attribute__( ( packed ) ) HashToValue {
    HashType hash = 0;
    Value value{};
};

/* @brief Writes read-only hash table of index records to file
 */
template <typename Value>
bool writeIndexFile( FILE* file, const std::vector<HashToValue<Value>>& hashesToValues ) {
    constexpr auto LoadFactor = 2;  // must be >= 1. It looks, 2 is optimal for taken flash space and performance

    std::vector<RecordInfo> recordsInfo( hashesToValues.size() * LoadFactor );

    if( recordsInfo.size() >= std::numeric_limits<IndexRecordNumber>::max() ) {
        assert( false );
        return false;
    }

    std::vector<bool> recordCollisions( hashesToValues.size() );

    for( IndexRecordNumber i = 0; i < hashesToValues.size(); ++i ) {
        const auto& hashToValue = hashesToValues[i];
        auto& currentRecordInfo = recordsInfo[hashToValue.hash % recordsInfo.size()];

        if( currentRecordInfo.number == 0 ) {
            currentRecordInfo.hashToValueIndex = i;
            currentRecordInfo.number = 1;
        }
        else {
            recordCollisions[i] = true;
        }
    }

    for( IndexRecordNumber i = 0; i < hashesToValues.size(); ++i ) {
        if( !recordCollisions[i] )
            continue;

        const auto& hashToValue = hashesToValues[i];

        const auto recordNumber = static_cast<IndexRecordNumber>( hashToValue.hash % recordsInfo.size() );
        auto currentRecordNumber = recordNumber;
        auto lastRecordNumber = currentRecordNumber;

        auto detectHashCollision = [&]() {
            if( hashToValue.hash == hashesToValues[recordsInfo[currentRecordNumber].hashToValueIndex].hash ) {
                ++recordsInfo[currentRecordNumber].number;
            }
        };

        detectHashCollision();

        while( true ) {
            currentRecordNumber = ( currentRecordNumber + 1 ) % recordsInfo.size();

            if( recordsInfo[currentRecordNumber].number == 0 )
                break;

            if( recordNumber ==
                hashesToValues[recordsInfo[currentRecordNumber].hashToValueIndex].hash % recordsInfo.size() ) {
                lastRecordNumber = currentRecordNumber;
                detectHashCollision();
            }
        }

        recordsInfo[lastRecordNumber].nextRecordNumber = currentRecordNumber;
        recordsInfo[currentRecordNumber].hashToValueIndex = i;
        recordsInfo[currentRecordNumber].number = 1;
    }

    for( const auto& recordInfo : recordsInfo ) {
        IndexRecord<Value> record;

        if( recordInfo.number ) {
            record.number = recordInfo.number;
            record.nextRecordNumber = recordInfo.nextRecordNumber;
            record.hash = hashesToValues[recordInfo.hashToValueIndex].hash;
            record.value = hashesToValues[recordInfo.hashToValueIndex].value;
        }

        if( fwrite( &record, sizeof( IndexRecord<Value> ), 1, file ) == 0 )
            return false;
    }

    return true;
}

/* @brief Finds all values for passed hasn in file that was written by function writeIndexFile.
 *        Time complexity O(1)
 */
template <typename Value>
bool findValuesInIndexFile( FILE* file, HashType hash, std::vector<Value>& values, IndexRecordNumber recordsNumber ) {
    if( recordsNumber == 0 || recordsNumber == InvalidRecordNumber ) {
        return false;
    }

    values.clear();

    auto currentRecordNumber = static_cast<IndexRecordNumber>( hash % recordsNumber );
    auto cursor = std::numeric_limits<IndexRecordNumber>::max();

    IndexRecord<Value> record;

    do {
        if( currentRecordNumber > cursor )
            fseek( file,
                   ( currentRecordNumber - cursor ) * sizeof( record ),
                   SEEK_CUR );  // usually offset is 1-3, so relative seek is more fast
        else if( currentRecordNumber < cursor )
            fseek( file, currentRecordNumber * sizeof( record ), SEEK_SET );

        fread( &record, sizeof( record ), 1, file );

        if( record.number == 0 )
            break;

        if( record.hash == hash ) {
            if( values.empty() )
                values.reserve( record.number );

            values.push_back( record.value );

            if( record.number == 1 )
                break;
        }

        cursor = currentRecordNumber + 1;
        currentRecordNumber = record.nextRecordNumber;
    } while( currentRecordNumber != InvalidRecordNumber );

    return true;
}

template <typename Value>
IndexRecordNumber getRecordsNumber( FILE* file ) {
    fseek( file, 0L, SEEK_END );

    const auto size = ftell( file ) / sizeof( IndexRecord<Value> );

    return size >= std::numeric_limits<IndexRecordNumber>::max() ? InvalidRecordNumber :
                                                                   static_cast<IndexRecordNumber>( size );
}

}  // namespace FileIndexation

}  // namespace common
