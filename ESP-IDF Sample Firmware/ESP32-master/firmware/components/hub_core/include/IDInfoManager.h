#pragma once
#include <ProvisionData.h>

namespace core {

namespace provision {
static constexpr auto SYNC_API_CALL_FORMAT = "api/v1/controller/sync?version=%u";

SyncRes syncProvisionData();

bool updateProvisionData( const ProvisionData& data, int& config_rev );

bool validateProvisionData( const ProvisionData& data );
}  // namespace provision

}  // namespace core
