#include "FeatureFlagController.h"

#include "BusCommandFeatureFlag.hpp"
#include "DebugUtils.h"

namespace core {
namespace features {

bool FeatureFlagController::initializeImpl() {
    if( !core::SingletonBaseImpl<FeatureFlagController>::initializeImpl() ) {
        return false;
    }
#ifdef LOCAL_MODE
    setDefaultFlag( FeatureFlag{ FeatureFlag::FT_LOCAL_MODE, "1.0", true } );
#else
    setDefaultFlag( FeatureFlag{ FeatureFlag::FT_LOCAL_MODE, "1.0", false } );
#endif
#ifdef CONFIG_CLOUD_DEVICES
    setDefaultFlag( FeatureFlag{ FeatureFlag::FT_CLOUD_DEVICES, "1.0", true } );
#else
    setDefaultFlag( FeatureFlag{ FeatureFlag::FT_CLOUD_DEVICES, "1.0", false } );
#endif
#ifdef CONFIG_ADVANCED_SCENES
    setDefaultFlag( FeatureFlag{ FeatureFlag::FT_ADVANCED_SCENES, "1.0", true } );
#else
    setDefaultFlag( FeatureFlag{ FeatureFlag::FT_ADVANCED_SCENES, "1.0", false } );
#endif
    return true;
}

std::vector<FeatureFlag> FeatureFlagController::getFeatureFlagList() const {
    return { getFeatureFlag( FeatureFlag::FT_LOCAL_MODE ),
             getFeatureFlag( FeatureFlag::FT_CLOUD_DEVICES ),
             getFeatureFlag( FeatureFlag::FT_ADVANCED_SCENES ) };
}

FeatureFlag FeatureFlagController::getFeatureFlag( FeatureFlag::FeatureType type ) const {
    auto feature_name = FeatureFlag::typeStr( type );
    if( m_storage->exists( feature_name ) ) {
        int8_t enabled = 0;
        if( m_storage->getInt8( feature_name, enabled ) ) {
            return FeatureFlag{ type, "1.0", enabled != 0 };
        }
        else {
            FUNC_LOGE( "Cannot get value of %s feature flag: the value could not be retrieved", feature_name );
            return FeatureFlag{ type, "1.0", false };
        }
    }
    else {
        FUNC_LOGE( "Cannot get value of %s feature flag: value does not exist", feature_name );
        return FeatureFlag{ type, "1.0", false };
    }
}

bool FeatureFlagController::setFeatureFlag( FeatureFlag::FeatureType feature_type, bool status ) {
    if( !m_storage->setInt8( FeatureFlag::typeStr( feature_type ), status ) ) {
        FUNC_LOGE( "Cannot set feature flag %s: the value could not be stored", FeatureFlag::typeStr( feature_type ) );
        return false;
    }
    broadcastFeatureStatusUpdate( FeatureFlag{ feature_type, "1.0", status } );
    return true;
}

void FeatureFlagController::setDefaultFlag( const FeatureFlag& feature ) {
    if( !m_storage->setInt8( feature.typeStr(), feature.status() ) ) {  // TODO: must be fixed in future
        FUNC_LOGE( "Cannot set default feature flag: failed to store value to the storage" );
    }
}

void FeatureFlagController::broadcastFeatureStatusUpdate( FeatureFlag featureFlag ) {
    switch( featureFlag.type() ) {
    case FeatureFlag::FT_LOCAL_MODE:
        send(::bus::command::FEATURE_FLAG_BUS_CLIENT_NAME,
             std::make_unique<::bus::command::LocalModeFeatureStatusUpdate>( std::move( featureFlag ) ) );
        break;
    case FeatureFlag::FT_CLOUD_DEVICES:
        send(::bus::command::FEATURE_FLAG_BUS_CLIENT_NAME,
             std::make_unique<::bus::command::CloudDevicesFeatureStatusUpdate>( std::move( featureFlag ) ) );
        break;
    case FeatureFlag::FT_ADVANCED_SCENES:
        send(::bus::command::FEATURE_FLAG_BUS_CLIENT_NAME,
             std::make_unique<::bus::command::AdvancedScenesFeatureStatusUpdate>( std::move( featureFlag ) ) );
        break;
    default:
        FUNC_LOGW( "Broadcast of feature flag status set is not implemented for %s", featureFlag.typeStr() );
        break;
    }
}

}  // namespace features
}  // namespace core
