#include "JsonGenerator.h"
#include "LocalScene.h"

namespace scenes {

template <>
bool jsonify<LocalScene>( const StringType& scenesPath,
                          const LocalScene& scene,
                          common::JsonStreamGenerator& gen,
                          bool notify,
                          bool isOnlyId ) {
    auto fName = scenesPath + "/" + scene.storageId();
    auto fStream = storage::ReadFileStream::open( fName.c_str() );
    if( !fStream ) {
        return false;
    }

    bool is_success = gen.writeOpeningObject();
    {
        is_success = is_success && gen.writeProperty( tokens::SubId, scene.externStorId() );
        if( is_success && !isOnlyId ) {
            is_success = gen.writeProperty( tokens::Enabled, scene.isEnabled() );

            if( is_success && scene.isValidName() ) {
                is_success = gen.writeProperty( tokens::Name, scene.name() );
            }

            if( is_success ) {
                const auto& groupId = scene.groupId();
                bool isGroup = !groupId.empty();
                is_success = gen.writeProperty( tokens::IsGroup, isGroup );
                if( is_success && isGroup ) {
                    is_success = gen.writeProperty( tokens::GroupId, groupId );
                }
            }

            is_success = is_success && gen.writeProperty( tokens::ParentId, scene.roomId() );
            is_success = is_success && gen.writeProperty( tokens::UserNotifications, scene.usersToNotify() );
            is_success = is_success && gen.writeProperty( tokens::ScenesHouseModes, scene.houseModes() );
            is_success = is_success && gen.writeRawProperty( nullptr, *fStream );
            is_success = is_success && gen.writeProperty( tokens::SyncNotification, notify );
        }
    }
    is_success = is_success && gen.writeClosingSymbol();

    return is_success;
}

}  // namespace scenes