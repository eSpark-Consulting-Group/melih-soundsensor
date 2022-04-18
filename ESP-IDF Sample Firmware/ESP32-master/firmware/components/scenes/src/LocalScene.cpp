#include "LocalScene.h"

namespace scenes {

LocalScene::LocalScene( Uuid uuid,
                        StringType&& name,
                        const char* roomId,
                        const char* groupId,
                        UuidSet&& whenBlocks,
                        UuidList&& thenBlocks,
                        HouseModesList&& houseModes,
                        UserNotificationsList&& usersToNotify,
                        bool isEnabled,
                        ExecPolicy execPolicy )
: Scene<LocalScene>( uuid, std::move( name ), std::move( whenBlocks ), std::move( thenBlocks ), isEnabled, execPolicy )
, m_roomId( roomId )
, m_groupId( groupId )
, m_houseModes( std::move( houseModes ) )
, m_usersToNotify( std::move( usersToNotify ) ) {}

LocalScene::LocalScene( Uuid uuid,
                        const char* name,
                        const char* roomId,
                        const char* groupId,
                        UuidSet&& whenBlocks,
                        UuidList&& thenBlocks,
                        HouseModesList&& houseModes,
                        UserNotificationsList&& usersToNotify,
                        bool isEnabled,
                        ExecPolicy execPolicy )
: LocalScene( uuid,
              StringType( name ),
              roomId,
              groupId,
              std::move( whenBlocks ),
              std::move( thenBlocks ),
              std::move( houseModes ),
              std::move( usersToNotify ),
              isEnabled,
              execPolicy ) {}

bool LocalScene::checkHouseMode( const std::uint8_t modeId ) const {
    char buff[32];
    if( !snprintf( buff, sizeof( buff ), "%" PRIu8, modeId ) ) {
        return false;
    }
    return checkHouseMode( buff );
}

bool LocalScene::checkHouseMode( const StringType& mode ) const {
    auto hModes = houseModes();
    return std::find( hModes.begin(), hModes.end(), mode ) != hModes.end();
}

bool LocalScene::read( common::IReadStream& stream ) {
    if( stream.read( m_uuid ) && stream.read( m_persistentStorId ) && stream.read( m_name ) &&
        stream.read( m_roomId ) && stream.read( m_groupId ) && stream.read( m_whenBlocks ) &&
        stream.read( m_thenBlocks ) && stream.read( m_houseModes ) && stream.read( m_usersToNotify ) &&
        stream.read( m_isEnabled ) ) {
        // Added in EATT-2265
        stream.read( m_execPolicy );
        return true;
    }

    return false;
}

bool LocalScene::write( common::IWriteStream& stream ) const {
    return stream.write( m_uuid ) && stream.write( m_persistentStorId ) && stream.write( m_name ) &&
           stream.write( m_roomId ) && stream.write( m_groupId ) && stream.write( m_whenBlocks ) &&
           stream.write( m_thenBlocks ) && stream.write( m_houseModes ) && stream.write( m_usersToNotify ) &&
           stream.write( m_isEnabled ) && stream.write( m_execPolicy );
}

}  // namespace scenes
