#pragma once

#include "IGatewayUserDataStorage.h"
#include "MemCache.h"
#include "values/TableValue.h"
#include "IValueCast.h"

namespace gateways {

/**
 * Implements storage based on in-memory cache.
 */
class GatewayUserDataInMemoryStorage : public IGatewayUserDataStorage {
public:
    void setUserData( GatewayID gatewayId, std::unique_ptr<::common::TableValue> newFields ) override {
        if( !newFields ) {
            return;
        }

        auto cacheKey = getCacheKey( gatewayId );

        if( auto existingFields =
                ::common::iValueCast<::common::TableValue>(::cache::MemCache::get().getValueByKey( cacheKey ) ) ) {
            for( auto& field : *newFields ) {
                existingFields->insert( field.first, std::move( field.second ) );
            }
            ::cache::MemCache::get().setValueByKey( cacheKey, std::move( existingFields ) );
        }
        else {
            ::cache::MemCache::get().setValueByKey( cacheKey, std::move( newFields ) );
        }
    }

    std::unique_ptr<::common::TableValue> getUserData( GatewayID gatewayId ) const override {
        return ::common::iValueCast<::common::TableValue>(
            ::cache::MemCache::get().getValueByKey( getCacheKey( gatewayId ) ) );
    }

private:
    static std::string getCacheKey( GatewayID gatewayId ) {
        return ::gateways::toString( gatewayId ) + std::string{ "_fields" };
    }
};

}  // namespace gateways
