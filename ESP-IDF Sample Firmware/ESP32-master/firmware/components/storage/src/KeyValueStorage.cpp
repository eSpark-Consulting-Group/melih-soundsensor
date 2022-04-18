#include "KeyValueStorage.h"

#include "IRamBufStream.h"
#include "IValueVisitor.h"
#include "Utils.h"
#include "DebugUtils.h"

#include <cstdio>

namespace storage {

common::IValue::UPtr KeyValueStorage::getIValue( const std::string& key ) {
    BlobType blob;
    if( !getBlob( key, blob ) || blob.empty() ) {
        return nullptr;
    }
    common::ArrayRStream stream( blob.data(), blob.size() );
    return common::IValue::create( stream );
}

bool KeyValueStorage::set( const std::string& key, const common::IValue& value ) {
    common::IRamBufferStream stream{ value.calculateSerializedSize() };
    return stream && stream.write( value ) && setBlob( key, stream, stream.size() );
}

const char* INT8_TYPE_NAME = "int8";
const char* INT16_TYPE_NAME = "int16";
const char* INT32_TYPE_NAME = "int32";
const char* INT64_TYPE_NAME = "int64";
const char* BLOB_TYPE_NAME = "blob";
const char* UNKNOW_TYPE_NAME = "Unknow";

const char* KeyValueStorage::ValueT::typeName() const {
    switch( val_type ) {
    case INT_8:
        return INT8_TYPE_NAME;
    case INT_16:
        return INT16_TYPE_NAME;
    case INT_32:
        return INT32_TYPE_NAME;
    case INT_64:
        return INT64_TYPE_NAME;
    case BLOB:
        return BLOB_TYPE_NAME;
    default:
        return UNKNOW_TYPE_NAME;
    }
}

void KeyValueStorage::printInfo() const {
    FUNC_LOGD( "------------Storage info--------------" );
    auto end = cend();
    for( auto itr = cbegin(); itr != end; ++itr ) {
        auto key = *itr;
        auto val = itr.value();
        FUNC_LOGD( "  key=%s type=%s\n", key.c_str(), val.typeName() );
    }
    FUNC_LOGD( "--------------------------------------" );
}

}  // namespace storage
