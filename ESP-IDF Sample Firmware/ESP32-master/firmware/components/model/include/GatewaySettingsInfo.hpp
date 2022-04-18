#pragma once

#include <SettingsInfo.h>

#include <GatewayDefs.h>
#include <GatewayID.h>
#include <Utils.h>
#include <SerializationBuffer.h>

#include "IValue.h"
#include "IValueCast.h"
#include "values/UserCodeValue.h"

#include <memory>
#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <functional>
#include <unordered_map>
#include <MemCache.h>
#include "CommonResources.h"
#include <DebugUtils.h>
#include "SettingStorage.h"

namespace common {
class IValue;
};

namespace gateways {

class GatewaySettingsInfo : public SettingsInfo {
    friend class ObjectSerializer;

public:
    GatewaySettingsInfo() = default;

    using UPtr = std::unique_ptr<GatewaySettingsInfo>;

    GatewaySettingsInfo( StringType settingId,
                         StringType gatewayId,
                         LabelType label,
                         ItemValueType valueType,
                         std::optional<ItemValueType> elementType )
    : SettingsInfo( std::move( settingId ), std::move( gatewayId ), label, valueType, elementType ){};

    common::IValue::UPtr getValue() const override {
        auto value = ::storage::GatewaySettings::get().getValue( getId() );
        return value ? std::move( value ) : createDefaultValue();
    }

    common::IValue::UPtr getValueCache() const override {
        return ::storage::GatewaySettings::get().getValue( getId() );
    }

    void setValue( std::unique_ptr<common::IValue> value ) override {
        ::storage::GatewaySettings::get().setValue( getId(), std::move( value ) );
    }
};

}  // namespace gateways
