#pragma once

#include <vector>

#include "FeatureFlag.hpp"
#include "KeyValueStorageFactory.h"
#include "Shutdownable.h"

namespace core {
namespace features {

/**
 * Covers business logic layer of feature flags implementation.
 * Provides interface for get/set/list operations.
 * All interactions with feature flags should be performed using a single instance of this class.
 *
 * fyi: feature flags are used to enable remote control of certain hub features in a unified way.
 *      hub.feature.status.set allows to toggle specific features.
 *      hub.features.list allows to request full list of supported features.
 */
class FeatureFlagController : public SingletonBaseImpl<FeatureFlagController> {
    friend class SingletonBaseImpl<FeatureFlagController>;

public:
    using ParentType = core::SingletonBaseImpl<FeatureFlagController>;
    using ClbkRet = ParentType::ClbkRet;

    std::vector<FeatureFlag> getFeatureFlagList() const;

    FeatureFlag getFeatureFlag( FeatureFlag::FeatureType ) const;

    bool setFeatureFlag( FeatureFlag::FeatureType, bool status );

private:
    bool initializeImpl() override;

    ClbkRet updateComponent( UpdContext& ctx ) override {
        return ClbkRet::Continue;
    }

    void setDefaultFlag( const FeatureFlag& );
    void broadcastFeatureStatusUpdate( FeatureFlag featureFlag );

    /**
     * Initializes default feature flags.
     */

    template <typename DelayType>
    FeatureFlagController(::storage::KeyValueStorageFactory::StoragePtr&& stor, const DelayType& delay )
    : core::SingletonBaseImpl<FeatureFlagController>( delay )
    , m_storage( std::move( stor ) ) {}

private:
    ::storage::KeyValueStorageFactory::StoragePtr m_storage;
};

}  // namespace features
}  // namespace core
