#pragma once

#include <string>

namespace time_service {

const std::string getTimeZoneId();

bool isDefaultTimezone();

bool setTzEnvVar( const std::string tzLocation, const std::string tzVar );

const std::string getTzEnvVariable();

}  // namespace time_service
