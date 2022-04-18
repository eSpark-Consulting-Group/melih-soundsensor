#pragma once

#include <chrono>

namespace time_service {

bool waitingTimeDidEstablished();

bool waitingTimeDidEstablishedFor( const std::chrono::milliseconds& delay );

}  // namespace time_service
