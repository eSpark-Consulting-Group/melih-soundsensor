#pragma once

#include <functional>
#include <string>

namespace storage {

class Storage {
    Storage() = default;

public:
    enum class WalkDir { Continue, Complete };

    using WalkDirClbk = std::function<WalkDir( char* name, char* full )>;

    static Storage& get();

    bool isExist( const char* filePath );

    inline bool isExist( const std::string& filePath ) {
        return isExist( filePath.c_str() );
    }

    bool remove( const char* filePath );

    inline bool remove( const std::string& filePath ) {
        return remove( filePath.c_str() );
    }

    bool rename( const char* oldFilePath, const char* newFilePath );

    bool walkDir( const char* dirPath, WalkDirClbk onFileClbk, WalkDirClbk onDirClbk );

    inline bool walkDir( const std::string& dirPath, WalkDirClbk onFileClbk, WalkDirClbk onDirClbk ) {
        return walkDir( dirPath.c_str(), onFileClbk, onDirClbk );
    }

    bool createDir( const char* dirPath );

    inline bool createDir( const std::string& dirPath ) {
        return createDir( dirPath.c_str() );
    }

    void printDir( const char* dirPath );
};

};  // namespace storage
