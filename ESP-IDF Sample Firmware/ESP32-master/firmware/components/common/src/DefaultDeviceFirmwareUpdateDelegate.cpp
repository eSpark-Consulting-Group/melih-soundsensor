#include "DefaultDeviceFirmwareUpdateDelegate.h"
#include <unistd.h>
#include <mbedtls/sha1.h>

#include <DebugUtils.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

namespace common {

DefaultDownloader::DefaultDownloader( const common::DeviceFirmwareUpdateManager::ExtendedLink& sourceLink,
                                      const common::DeviceFirmwareUpdateManager::Path& destinationPath,
                                      ResultCallback resultcallback )
: m_request( sourceLink.image ) {
    FUNC_LOGI( "Download image file" );
    const auto imageDestinationPath = destinationPath;
    m_request.setDownloadPath( imageDestinationPath );

    FUNC_LOGI( "imageDestinationPath: %s", imageDestinationPath.c_str() );

    m_request.setReceiveHandler( [&]( int status ) {
        FUNC_LOGI( "setReceiveHandler status: %d", status );
        m_isOk = status == 200;
        resultcallback( *this );
    } );

    m_request.setConnectCloseHandler( [&]() {
        if( m_progress != 100 ) {
            resultcallback( *this );
        }
    } );

    m_request.setConnectDownloadingProgressHandler( [&]( uint8_t progress ) {
        if( progress % 10 == 0 && progress != m_progress ) {
            FUNC_LOGI( "progress: %d", progress );
            m_progress = progress;
            resultcallback( *this );
        }
    } );

    if( m_request.send() != 200 ) {
        m_isOk = false;
        resultcallback( *this );
        return;
    }

    if( m_progress != 100 ) {
        resultcallback( *this );
    }

    FUNC_LOGI( "Download checksum file" );
    m_request.setUrl( sourceLink.checksum );
    const auto checksumDestinationPath = destinationPath + "checksum.sh1";
    m_request.setDownloadPath( checksumDestinationPath );
    FUNC_LOGI( "checksumDestinationPath: %s", checksumDestinationPath.c_str() );
    m_request.setReceiveHandler( [&]( int status ) {} );

    if( m_request.send() != 200 ) {
        m_isOk = false;
        resultcallback( *this );
    }
    else {
        if( m_progress == 100 ) {
            if( checksumCompare( imageDestinationPath.c_str(), checksumDestinationPath.c_str() ) ) {
                resultcallback( *this );
                return;
            }
        }
    }

    m_isOk = false;
    resultcallback( *this );
}

std::string toHex( unsigned char* text, size_t size ) {
    std::string result;
    if( text && *text && size > 0 ) {
        result.reserve( size * 2 + 1 );

        char buf[4];
        for( size_t i = 0; i < size; i++ ) {
            snprintf( buf, sizeof( buf ), "%02x", text[i] );
            result += buf;
        }
    }
    return result;
}

bool DefaultDownloader::checksumCompare( const char* destinationPath, const char* checksumDestinationPath ) {
    bool res = false;
    std::ifstream check( checksumDestinationPath );  // taking file as inputstream
    std::string str;
    if( check ) {
        std::ostringstream ss;
        ss << check.rdbuf();  // reading data
        str = ss.str();
    }

    std::ifstream myfile( destinationPath, std::ifstream::in );

    std::ifstream stream( destinationPath, std::ifstream::binary );
    const int SHA_DIGEST_LENGTH = 20;
    if( stream ) {
        const int BLOCK_SIZE = 1024;
        mbedtls_sha1_context ctx = {};
        unsigned char digest[20];

        char buffer[BLOCK_SIZE];

        mbedtls_sha1_init( &ctx );
        mbedtls_sha1_starts( &ctx );

        while( stream ) {
            stream.read( buffer, BLOCK_SIZE );
            mbedtls_sha1_update( &ctx, (unsigned char*)buffer, stream.gcount() );
        }
        mbedtls_sha1_finish( &ctx, digest );
        mbedtls_sha1_free( &ctx );

        const auto digestStr = toHex( digest, SHA_DIGEST_LENGTH );
        str.erase( std::remove( str.begin(), str.end(), '\n' ), str.end() );
        res = digestStr == str;
    }
    return res;
}

bool DefaultDownloader::isOk() const {
    FUNC_LOGI( " m_isOk: %d", m_isOk );
    return m_isOk;
}

uint8_t DefaultDownloader::getProgress() const {
    FUNC_LOGI( "m_progress: %d", m_progress );
    return m_progress;
}

bool DefaultDownloader::isLink( const DeviceFirmwareUpdateManager::Link& link ) {
    const std::string& linkMarker = "http";
    return link.substr( 0, linkMarker.size() ) == linkMarker;
}

DeviceFirmwareUpdateManager::DownloaderConstUPtr DefaultDeviceFirmwareUpdateDelegate::createFirmwareDownloader(
    const DeviceFirmwareUpdateManager::ExtendedLink& link,
    const DeviceFirmwareUpdateManager::Path& destinationPath,
    DeviceFirmwareUpdateManager::Downloader::ResultCallback resultCallback ) {
    return std::make_unique<DefaultDownloader>( link, destinationPath, std::move( resultCallback ) );
}

DeviceFirmwareUpdateManager::Path DefaultDeviceFirmwareUpdateDelegate::getDestinationForLink(
    const DeviceFirmwareUpdateManager::Link& link ) {
    char fileName[] = "/vol/node_fw.bin";
    FUNC_LOGI( "fileName (path): %s", fileName );
    return fileName;
}

std::string getPath( const std::string& filePath ) {
    auto filePos = filePath.find_last_of( '/' );
    return filePath.substr( 0, filePos );
}

}  // namespace common
