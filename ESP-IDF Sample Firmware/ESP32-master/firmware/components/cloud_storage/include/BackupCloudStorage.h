#include <string>
#include <chrono>
#include <functional>
#include <map>
#include "gason.hpp"
#include "HTTPRequest.h"

namespace cloud_storage {

enum class StorageType {
    Backup,
    Multimedia,
    ControllerBackup,
    ArchiveControllerBackup,
    ControllerLog,
    ArchiveControllerLog
};
enum class OwnerEntityType { Controller, User, Location };

class BackupCloudStorage {
public:
    BackupCloudStorage() = default;
    BackupCloudStorage( StorageType storageType )
    : m_storageType( storageType ) {}

    ~BackupCloudStorage() = default;

    bool uploadFile( const std::string& file, const std::string& metadata = "" );
    bool downloadFile( const std::string& downloadPath );

private:
    std::string toString( StorageType storageType );
    std::string toString( OwnerEntityType ownerEntityType );
    std::string contentTypeByExtension( std::string extension );

private:
    StorageType m_storageType = StorageType::ArchiveControllerBackup;
};
}  // namespace cloud_storage
