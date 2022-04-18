#include "INmaJsonHandler.h"
#include "HardUtils.h"

namespace nma {

INmaJsonHandler::INmaJsonHandler()
: m_msgId( hard::common::generateUniqueId() ) {}

}  // namespace nma
