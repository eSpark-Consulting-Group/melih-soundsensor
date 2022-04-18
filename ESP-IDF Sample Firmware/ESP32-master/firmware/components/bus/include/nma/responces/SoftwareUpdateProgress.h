#pragma once

#include "BusCommand.h"

#include <string>
#include <memory>

namespace nma {
namespace responces {

// See https://log.ezlo.com/new/hub/broadcasts/#hubsoftwareupdateprogress

class HubSoftwareUpdateProgress : public bus::IBusCommand {
public:
    using UPtr = std::unique_ptr<HubSoftwareUpdateProgress>;
    using StringType = std::string;
    enum class Operations { NotDefined, Install, Upgrade, Sysupgrade, Uninstall };
    enum class Status { Started, Finished };
    enum class ActionType { Firmware, Addon, Plugin };

    static auto createPluginInstall( StringType pluginUri,
                                     StringType version,
                                     const Status status,
                                     const int progress,
                                     const bool isError = false ) {
        return std::make_unique<HubSoftwareUpdateProgress>( StringType(),
                                                            status,
                                                            isError,
                                                            ActionType::Plugin,
                                                            std::move( pluginUri ),
                                                            Operations::Install,
                                                            std::move( version ),
                                                            progress );
    }

    static auto createPluginUninstall( StringType pluginUri,
                                       StringType version,
                                       const Status status,
                                       const int progress,
                                       const bool isError = false ) {
        return std::make_unique<HubSoftwareUpdateProgress>( StringType(),
                                                            status,
                                                            isError,
                                                            ActionType::Plugin,
                                                            std::move( pluginUri ),
                                                            Operations::Uninstall,
                                                            std::move( version ),
                                                            progress );
    }

    HubSoftwareUpdateProgress( StringType&& operationId,
                               const Status status,
                               const bool isError,
                               const ActionType actionType,
                               StringType&& actionId,
                               const Operations actionOperation,
                               StringType&& actionVersion,
                               int operationCompletedProgress )
    : m_operationId( std::move( operationId ) )
    , m_status( status )
    , m_IsError( isError )
    , m_actionType( actionType )
    , m_actionId( actionId )
    , m_actionOperation( actionOperation )
    , m_actionVersion( std::move( actionVersion ) )
    , m_operationCompletedProgress( operationCompletedProgress ) {}

    inline const auto& operationId() const {
        return m_operationId;
    }

    inline const auto status() const {
        return m_status;
    }

    StringType statusString() const {
        return Status::Started == m_status ? "started" : "finished";
    }

    inline auto isError() const {
        return m_IsError;
    }

    StringType actionTypeString() const {
        switch( m_actionType ) {
        case ActionType::Firmware:
            return "firmware";
        case ActionType::Addon:
            return "addon";
        case ActionType::Plugin:
            return "plugin";
        default:
            return "unknow";
        }
    }

    inline const auto& actionType() const {
        return m_actionType;
    }

    inline const auto& actionId() const {
        return m_actionId;
    }

    inline bool isAction() const {
        return Operations::NotDefined == Operations::NotDefined;
    }

    StringType actionOperationString() const {
        switch( m_actionOperation ) {
        case Operations::Install:
            return "install";
        case Operations::Upgrade:
            return "upgrade";
        case Operations::Sysupgrade:
            return "sysupgrade";
        case Operations::Uninstall:
            return "uninstall";
        default:
            return "unknow";
        }
    }

    inline auto actionOperation() const {
        return m_actionOperation;
    }

    inline const auto& actionVersion() const {
        return m_actionVersion;
    }

    inline auto operationCompletedProgress() const {
        return m_operationCompletedProgress;
    }

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

private:
    StringType m_operationId;  // ID of message
    Status m_status;
    bool m_IsError;
    ActionType m_actionType;
    StringType m_actionId;
    Operations m_actionOperation = Operations::NotDefined;
    StringType m_actionVersion;        // Version of referenced software
    int m_operationCompletedProgress;  // Progress of operation
};

}  // namespace responces
}  // namespace nma
