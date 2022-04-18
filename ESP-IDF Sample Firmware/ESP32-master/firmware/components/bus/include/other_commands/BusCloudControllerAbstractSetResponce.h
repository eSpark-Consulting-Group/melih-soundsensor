#pragma once

#include "BusCommand.h"

#include <string>

namespace bus {
namespace command {
namespace nma {

class CloudControllerAbstractSetResponce final : public IBusCommand {
    std::string m_id;
    bool m_isSuccess = true;

public:
    CloudControllerAbstractSetResponce( const char* id, bool isSuccess )
    : m_id( id )
    , m_isSuccess( isSuccess ) {}

    inline auto isSuccess() const {
        return m_isSuccess;
    }

    inline const auto& id() const {
        return m_id;
    }

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) {
        client.visit( from, to, *this );
    }
};

}  // namespace nma
}  // namespace command
}  // namespace bus
