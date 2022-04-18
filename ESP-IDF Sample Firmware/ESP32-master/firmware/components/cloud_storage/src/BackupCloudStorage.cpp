#include "BackupCloudStorage.h"
#include "ControllerStorageTokenInfo.h"
#include "BackupFileInfo.h"

namespace cloud_storage {

using namespace network;
using namespace core;

bool BackupCloudStorage::uploadFile( const std::string& filename, const std::string& metadata ) {
    std::string name;
    std::string extension;
    std::string baseName = filename.substr( filename.find_last_of( "/\\" ) + 1 );

    if( !baseName.length() ) {
        FUNC_LOGE( "Incortrect file path" );
        return false;
    }

    auto idx = baseName.rfind( '.' );
    if( idx != std::string::npos ) {
        extension = baseName.substr( idx + 1 );
        name = baseName.substr( 0, idx );
    }
    else {
        FUNC_LOGE( "Incortrect file name" );
        return false;
    }

    core::ControllerStorageToken storageInfo;
    if( !storageInfo.load() ) {
        FUNC_LOGE( "Cant load storage" );
        return false;
    }

    const std::string token = storageInfo.tokenString();
    const std::string xapp = storageInfo.xapp();
    const auto url = storageInfo.uploadEndpoints().front();
    std::map<std::string, std::string> headers = {
        { "p-type", toString( m_storageType ) }, { "p-filename", name },
        { "p-file_format", extension },          { "X-APP", xapp },
        { "Authorization", "Bearer " + token },
    };

    if( !metadata.empty() ) {
        headers.insert( std::pair<std::string, std::string>( "p-meta", metadata ) );
    }

    HTTPRequest req( url, HTTP_METHOD_POST );

    req.setSkipSecurity( false );
    req.setHeaders( headers );
    req.setContentType( contentTypeByExtension( extension ) );
    req.uploadFile( filename, [filename]( const char* storageKeyInfo ) {
        BackupFileInfo info;
        info.setLocalPath( filename );
        info.setBackupString( storageKeyInfo );
        info.save( filename );
    } );

    return true;
}

bool BackupCloudStorage::downloadFile( const std::string& downloadPath ) {
    core::ControllerStorageToken storageInfo;
    if( !storageInfo.load() ) {
        FUNC_LOGE( "Cant load storage" );
        return false;
    }

    const auto token = storageInfo.tokenString();
    const auto xapp = storageInfo.xapp();
    const auto url = storageInfo.downloadEndpoints().front();

    network::HTTPRequest req( url, HTTP_METHOD_POST );
    std::map<std::string, std::string> headers = {
        { "X-APP", xapp },
        { "Authorization", "Bearer " + token },
    };

    std::string backupInfo = BackupFileInfo::backupInfoForFile( downloadPath );

    if( !backupInfo.empty() ) {
        const auto firstDelim = backupInfo.find_first_of( ":" );
        std::string backupItemInfo = backupInfo.substr( firstDelim + 1, backupInfo.length() - firstDelim - 2 );
        if( !backupItemInfo.empty() ) {
            req.setContentType( "application/json" );
            req.setContent( backupItemInfo );
            req.setHeaders( headers );
            req.setDownloadPath( downloadPath );
            req.send();

            return true;
        }
    }

    return false;
}

std::string BackupCloudStorage::toString( StorageType storageType ) {
    switch( storageType ) {
    case StorageType::Backup:
        return "backup";
    case StorageType::Multimedia:
        return "multimedia";
    case StorageType::ControllerBackup:
        return "controller_backup";
    case StorageType::ArchiveControllerBackup:
        return "archive_controller_backup";
    case StorageType::ControllerLog:
        return "controller_log";
    case StorageType::ArchiveControllerLog:
        return "archive_controller_log";
    }
    return std::string();
    ;
}

std::string BackupCloudStorage::toString( OwnerEntityType ownerEntityType ) {
    switch( ownerEntityType ) {
    case OwnerEntityType::User:
        return "user";
    case OwnerEntityType::Location:
        return "location";
    case OwnerEntityType::Controller:
        return "controller";
    }
    return std::string();
    ;
}

std::string BackupCloudStorage::contentTypeByExtension( std::string extension ) {
    if( extension == "jpg" ) {
        return "image/jpeg";
    }
    else if( extension == "png" ) {
        return "image/png";
    }
    else if( extension == "mp4" ) {
        return "video/mp4";
    }
    else if( extension == "tgz" ) {
        return "application/tar+gzip";
    }
    else if( extension == "zip" ) {
        return "application/zip";
    }
    else if( extension == "svg" ) {
        return "image/svg+xml";
    }
    else if( extension == "json" ) {
        return "application/json";
    }
    else if( extension == "mjpg" ) {
        return "video/x-motion-jpeg";
    }
    else if( extension == "mkv" ) {
        return "video/x-matroska";
    }

    return "application/octet-stream";
}

}  // namespace cloud_storage
