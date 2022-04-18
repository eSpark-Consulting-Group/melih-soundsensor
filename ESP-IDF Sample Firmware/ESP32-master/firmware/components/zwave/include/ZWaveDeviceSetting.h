
#pragma once

#include <znet/iznet_node_setting.hpp>
#include "values/TableValue.h"
#include <JsonListener.h>
#include <functional>

namespace gateways {
namespace zwave {

class NodeSetting : public znet::INodeSetting {
public:
    NodeSetting( std::string path, uint16_t manufacturerId, uint16_t productType, uint16_t productId );
    NodeSetting() = delete;

    void forEachConfiguration( std::function<void( const znet::Configuration& )> callback ) const override;
    void forEachAssociation( std::function<void( const znet::Association& )> callback ) const override;

    znet::utils::optional<uint32_t> getWakeUpInterval() const override;
    znet::utils::optional<bool> shouldRequestNotifications() const override;
    znet::utils::optional<bool> isExtendedUserCodesRequestRestricted() const override;

private:
    void getFieldValue( std::function<void( jssp::JsonListenerDeviceSetting& )> getter ) const;

    std::string m_archiveName;
    std::string m_fileName;
    uint16_t m_manufacturerId;
    uint16_t m_productType;
    uint16_t m_productId;
};

}  // namespace zwave
}  // namespace gateways
