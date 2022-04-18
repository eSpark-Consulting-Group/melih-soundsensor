#include "BackupFileInfo.h"
#include "CommonResources.h"
#include <BinaryFileStorageFactory.h>
#include <StorageSection.h>
#include "IRamBufStream.h"

#include "DebugUtils.h"

namespace core {

using BufferStreamType = common::IRamBufferStream;
static constexpr const char* ControllerStorageFilesKey = "ControllerStorageFilesKey";

bool BackupFileInfo::read( common::IReadStream& stream ) {
    auto res = stream.read( m_path ) && stream.read( m_backupString );

    return res;
}

bool BackupFileInfo::write( common::IWriteStream& stream ) const {
    return stream.write( m_path ) && stream.write( m_backupString );
}

bool BackupFileInfo::load( const std::string& id, std::vector<char>&& buff, BackupFileInfo& object ) {
    auto stor =
        storage::BinaryFileStorageFactory::get().openNamespace( storage::section::CONTROLLER_STORAGE_FILES_INFO );

    if( !stor ) {
        return false;
    }

    storage::KeyValueStorage::BlobType blob;
    if( stor->getBlob( id, blob ) ) {
        common::ArrayRStream stream( blob.data(), blob.size() );
        if( object.read( stream ) ) {
            return true;
        }
        else {
            FUNC_LOGE( "Can't load hub settings from storage!" );
            return false;
        }
    }
    return false;
}

bool BackupFileInfo::save( std::string filename ) {
    BufferStreamType stream( BufferStreamType::MiddleBufSize );

    auto stor =
        storage::BinaryFileStorageFactory::get().openNamespace( storage::section::CONTROLLER_STORAGE_FILES_INFO );

    bool is_successfull = stream && write( stream ) && stor->setBlob( filename, stream, stream.size() );
    if( is_successfull ) {
        FUNC_LOGI( "Save controller storage token to storage - done" );
    }
    else {
        FUNC_LOGE( "Save controller storage token to storage - error" );
    }

    return is_successfull;
}

void BackupFileInfo::enumerateBackupFileInfoFromDb( std::function<bool( const BackupFileInfo& info )> clbk ) {
    using BlobType = storage::KeyValueStorage::BlobType;
    auto stor =
        storage::BinaryFileStorageFactory::get().openNamespace( storage::section::CONTROLLER_STORAGE_FILES_INFO );
    if( !stor ) {
        return;
    }

    BackupFileInfo info;

    auto end = stor->cend();
    for( auto it = stor->cbegin(); end != it; ++it ) {
        auto val = it.value();

        if( !val.is_type<BlobType>() || val.get<BlobType>().empty() ||
            !load( *it, std::move( (std::vector<char>)val.get<BlobType>() ), info ) ) {
            return;
        }

        if( !clbk( info ) ) {
            return;
        }
    }
}

std::string BackupFileInfo::backupInfoForFile( std::string file ) {
    std::string res;
    enumerateBackupFileInfoFromDb( [&file, &res]( const BackupFileInfo& info ) {
        if( info.localPath() == file ) {
            res = info.backupString();
            return false;
        }
        return true;
    } );

    return res;
}

}  // namespace core
