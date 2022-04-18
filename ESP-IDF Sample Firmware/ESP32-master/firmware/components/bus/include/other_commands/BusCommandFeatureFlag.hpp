#pragma once

#include "Bus.h"
#include "FeatureFlag.hpp"

/**
 * This file contains bus commands used to broadcast updates of feature flags.
 * See FeatureFlagController class for details.
 */

namespace bus {
namespace command {

static constexpr auto FEATURE_FLAG_BUS_CLIENT_NAME = "feature_flag";

class FeatureStatusUpdate : public IBusCommand {
public:
    explicit FeatureStatusUpdate(::core::features::FeatureFlag featureFlag )
    : m_featureFlag( std::move( featureFlag ) ) {}

    ::core::features::FeatureFlag featureFlag() const {
        return m_featureFlag;
    }

private:
    ::core::features::FeatureFlag m_featureFlag;
};

class LocalModeFeatureStatusUpdate : public FeatureStatusUpdate {
public:
    using FeatureStatusUpdate::FeatureStatusUpdate;

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

class CloudDevicesFeatureStatusUpdate : public FeatureStatusUpdate {
public:
    using FeatureStatusUpdate::FeatureStatusUpdate;

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

class AdvancedScenesFeatureStatusUpdate : public FeatureStatusUpdate {
public:
    using FeatureStatusUpdate::FeatureStatusUpdate;

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

}  // namespace command
}  // namespace bus
