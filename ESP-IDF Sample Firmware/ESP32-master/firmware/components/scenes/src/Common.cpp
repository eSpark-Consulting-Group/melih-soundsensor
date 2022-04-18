#include "Common.h"

#include "BinaryFileStorageFactory.h"

#include <cstdio>
#include <cstdlib>

#include "DebugUtils.h"

namespace scenes {

const char LOCAL_SCENES_PATH[] = "/vol/scenes";
const char GLOBAL_SCENES_PATH[] = "/vol/gscenes";
const char SCENES_CACHE_PATH[] = "/vol/scenes_cache";

const char* LOCAL_STORAGE_DB_NAMESPACE = "SCENES_V2";
const char* GLOBAL_STORAGE_DB_NAMESPACE = "GLOB_SCENES_V1";

Uuid::Uuid( const std::string& str ) {
    auto pos = str.find( '_' );
    if( pos == str.length() - 1 ) {
        return;
    }
    char* end;

    m_uuid.uuid64 = strtoull( str.substr( pos + 1 ).c_str(), &end, 16 );
}

StorUPtr getLocalStorage() {
    auto storage = storage::BinaryFileStorageFactory::get().openNamespace( LOCAL_STORAGE_DB_NAMESPACE );
    if( !storage ) {
        FUNC_LOGE( "Can't open local scenes storage!" );
    }
    return storage;
}

StorUPtr getGlobalStorage() {
    auto storage = storage::BinaryFileStorageFactory::get().openNamespace( GLOBAL_STORAGE_DB_NAMESPACE );
    if( !storage ) {
        FUNC_LOGE( "Can't open global scenes storage!" );
    }
    return storage;
}

}  // end of namespace scenes
