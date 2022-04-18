#pragma once

#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "DebugUtils.h"

using namespace common::resources;

namespace core {

class BackupFileInfo {
public:
    BackupFileInfo(){};

    static bool load( const std::string& id, std::vector<char>&& buff, BackupFileInfo& object );
    bool save( std::string filename );

    void setBackupString( const std::string& backupString ) {
        m_backupString = backupString;
    }

    inline std::string backupString() const {
        return m_backupString;
    }

    inline void setLocalPath( const std::string& path ) {
        m_path = path;
    }

    inline std::string localPath() const {
        return m_path;
    }

    static std::string backupInfoForFile( std::string file );

    static void enumerateBackupFileInfoFromDb( std::function<bool( const BackupFileInfo& info )> clbk );

private:
    bool read( common::IReadStream& stream );
    bool write( common::IWriteStream& stream ) const;

private:
    std::string m_backupString;
    std::string m_path;
};

}  // namespace core
