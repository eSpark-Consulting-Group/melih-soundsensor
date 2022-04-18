#pragma once

#include "BusCommand.h"

namespace settings::responces {

class SettingDictionaryStatusUpdated : public ::bus::IBusCommand {
public:
    SettingDictionaryStatusUpdated( std::string settingId, std::string elementId, std::string status )
    : m_settingId( std::move( settingId ) )
    , m_elementId( std::move( elementId ) )
    , m_status( std::move( status ) ) {}

    void accept(::bus::BusAddr from, ::bus::BusAddr to, ::bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    const auto& getSettingId() const {
        return m_settingId;
    }

    const auto& getElementId() const {
        return m_elementId;
    }

    const auto& getStatus() const {
        return m_status;
    }

private:
    std::string m_settingId;
    std::string m_elementId;
    std::string m_status;
};

}  // namespace settings::responces
