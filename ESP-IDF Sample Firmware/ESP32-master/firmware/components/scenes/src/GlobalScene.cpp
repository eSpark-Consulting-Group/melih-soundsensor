#include "GlobalScene.h"

namespace scenes {

GlobalScene::GlobalScene( Uuid uuid,
                          StringType&& name,
                          StringType&& tag,
                          UuidSet&& whenBlocks,
                          UuidList&& thenBlocks,
                          bool isEnabled,
                          ExecPolicy execPolicy )
: Scene( uuid, std::move( name ), std::move( whenBlocks ), std::move( thenBlocks ), isEnabled, execPolicy )
, m_tag( std::move( tag ) ) {}

GlobalScene::GlobalScene( Uuid uuid,
                          const char* name,
                          StringType&& tag,
                          UuidSet&& whenBlocks,
                          UuidList&& thenBlocks,
                          bool isEnabled,
                          ExecPolicy execPolicy )
: GlobalScene( uuid,
               StringType( name ),
               std::move( tag ),
               std::move( whenBlocks ),
               std::move( thenBlocks ),
               isEnabled,
               execPolicy ) {}

bool GlobalScene::read( common::IReadStream& stream ) {
    return stream.read( m_uuid ) && stream.read( m_persistentStorId ) && stream.read( m_name ) &&
           stream.read( m_whenBlocks ) && stream.read( m_thenBlocks ) && stream.read( m_isEnabled ) &&
           stream.read( m_execPolicy ) && stream.read( m_tag );
}

bool GlobalScene::write( common::IWriteStream& stream ) const {
    return stream.write( m_uuid ) && stream.write( m_persistentStorId ) && stream.write( m_name ) &&
           stream.write( m_whenBlocks ) && stream.write( m_thenBlocks ) && stream.write( m_isEnabled ) &&
           stream.write( m_execPolicy ) && stream.write( m_tag );
}

}  // namespace scenes
