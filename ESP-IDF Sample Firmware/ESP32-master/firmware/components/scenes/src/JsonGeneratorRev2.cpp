#include "JsonGenerator.h"
#include "GlobalScene.h"

namespace scenes {

template <>
bool jsonify<GlobalScene>( const StringType& scenesPath,
                           const GlobalScene& scene,
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
            is_success = is_success && gen.writeProperty( tokens::Enabled, scene.isEnabled() );

            if( is_success && scene.isValidName() ) {
                is_success = gen.writeProperty( tokens::Name, scene.name() );
            }

            if( is_success && scene.isValidTag() ) {
                is_success = gen.writeProperty( tokens::Tag, scene.tag() );
            }

            is_success = is_success && gen.writeRawProperty( nullptr, *fStream );
            is_success = is_success && gen.writeProperty( tokens::SyncNotification, notify );
        }
    }
    is_success = is_success && gen.writeClosingSymbol();

    return is_success;
}

}  // namespace scenes