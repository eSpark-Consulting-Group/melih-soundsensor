#pragma once

#include <cstring>
#include <string>

namespace core {
namespace features {

/**
 * Describes the state of a given feature flag. See FeatureFlagController for details.
 */
class FeatureFlag {
public:
    // FT_ prefix is required to avoid conflicts with the corresponding #defines.
    enum FeatureType { FT_LOCAL_MODE, FT_CLOUD_DEVICES, FT_ADVANCED_SCENES };

    FeatureFlag( FeatureType type, std::string version, bool status )
    : m_type( type )
    , m_version( std::move( version ) )
    , m_status( status ) {}

    std::string version() const {
        return m_version;
    }

    bool status() const {
        return m_status;
    }

    const char* statusStr() const {
        return m_status ? "on" : "off";
    }

    FeatureType type() const {
        return m_type;
    }

    const char* typeStr() const {
        return typeStr( m_type );
    }

    static const char* typeStr( FeatureType type ) {
        switch( type ) {
        case FT_LOCAL_MODE:
            return "local_mode";
        case FT_CLOUD_DEVICES:
            return "cloud_devices";
        case FT_ADVANCED_SCENES:
            return "advanced_scenes";
        default:
            return "unknown_feature";
        }
    }

    /**
     * Converts feature type string to a constant of type FeatureFlag::FeatureType
     * @param type String representation of feature type.
     * @return Pair of values:
     *          - Status indicating whether parsing is successful.
     *          - Converted value in case of successful conversion, or a unspecified value otherwise.
     */
    static std::pair<bool, FeatureType> typeFromStr( const char* type ) {
        if( 0 == ::strcasecmp( type, "local_mode" ) ) {
            return std::make_pair( true, FeatureType::FT_LOCAL_MODE );
        }
        else if( 0 == ::strcasecmp( type, "cloud_devices" ) ) {
            return std::make_pair( true, FeatureType::FT_CLOUD_DEVICES );
        }
        else if( 0 == ::strcasecmp( type, "advanced_scenes" ) ) {
            return std::make_pair( true, FeatureType::FT_ADVANCED_SCENES );
        }
        else {
            return std::make_pair( false, FeatureType{} );
        }
    }

    static const char* statusStr( bool status ) {
        return status ? "on" : "off";
    }

    /**
     * Converts feature status string to a boolean value.
     * @param type String representation of feature status.
     * @return Pair of values:
     *          - Status indicating whether parsing is successful.
     *          - Converted value in case of successful conversion, or a unspecified value otherwise.
     */
    static std::pair<bool, bool> statusFromStr( const char* status ) {
        if( 0 == ::strcasecmp( status, "on" ) ) {
            return std::make_pair( true, true );
        }
        else if( 0 == ::strcasecmp( status, "off" ) ) {
            return std::make_pair( true, false );
        }
        else {
            return std::make_pair( false, bool{} );
        }
    }

private:
    FeatureType m_type;
    std::string m_version;
    bool m_status;
};

}  // namespace features
}  // namespace core
