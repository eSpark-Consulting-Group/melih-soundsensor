#pragma once

#include <Bus.h>
#include "BusCommandHandleEvent.h"

#include <memory>
#include <algorithm>

namespace gateways {
namespace zwave {

class Publisher {
public:
    virtual ~Publisher() = default;

    void subscribe( bus::BusAddr addr, std::string handlerName ) {
        m_subscribers[addr].push_back( handlerName );
    }

    void unsubscribe( bus::BusAddr addr, std::string handlerName ) {
        if( m_subscribers.find( addr ) != m_subscribers.end() ) {
            auto it = std::find( m_subscribers[addr].begin(), m_subscribers[addr].end(), handlerName );
            if( it != m_subscribers[addr].end() ) {
                m_subscribers[addr].erase( it );
                if( m_subscribers[addr].size() == 0 ) {
                    m_subscribers.erase( addr );
                }
            }
        }
    }

    bool isSubscribersEmpty() {
        return m_subscribers.size() == 0;
    }

protected:
    virtual void notify(
        std::function<std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>( std::string )> func ) = 0;

protected:
    std::map<bus::BusAddr, std::list<std::string>> m_subscribers;
};

}  // namespace zwave
}  // namespace gateways
