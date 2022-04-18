#pragma once

#include <string>
#include <list>
#include <map>
#include <BusCommand.h>

namespace bus {
namespace command {

struct FavoriteUpdated final : public IBusCommand {
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    std::string m_from;
    std::string m_to;

    enum class Operation { added, removed };

    Operation m_operation;
    using section = std::string;
    using value = std::list<std::string>;
    std::map<section, value> m_favoriteObject;
};

}  // namespace command
}  // namespace bus
