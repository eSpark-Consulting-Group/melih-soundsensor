#pragma once

#include "TmplCommon.h"
#include "TmplGenStateBase.h"

#include "IReadStream.h"
#include "DeviceInfo.h"

namespace scenes {

class TemplatesGenerator : public jssp::JsonParser<TmplGenState_Base, TemplatesGenerator> {
public:
    using BaseType = jssp::JsonParser<TmplGenState_Base, TemplatesGenerator>;

    TemplatesGenerator( common::JsonStreamGenerator& gen,
                        common::IReadStream::UPtr&& stream,
                        const gateways::BaseDeviceInfo& deviceInfo,
                        const std::list<gateways::ItemInfo>& itemInfoList );

    bool generate( const gateways::ItemInfo& itemInfo, const TemplateBlockData& blockData );

    inline auto getPos() const {
        return m_pos;
    }

    inline void storeState( StateData& state ) const {
        state.m_pos = getPos();
        BaseType::storeState( state.m_parserState );
    }

    inline void restoreState( const StateData& state ) {
        m_stateToRestore = &state;
    }

    inline auto& getGen() {
        return m_gen;
    }

    inline const auto& getData() const {
        return *m_data;
    }

    inline const auto& getItemInfo() const {
        return *m_itemInfo;
    }

    inline const auto& getDeviceInfo() const {
        return m_deviceInfo;
    }

    inline const auto& getItemsInfoList() const {
        return m_itemInfoList;
    }

    bool reset() override;

private:
    const TemplateBlockData* m_data;
    const gateways::ItemInfo* m_itemInfo;
    int m_pos;
    common::JsonStreamGenerator& m_gen;
    common::IReadStream::UPtr m_stream;
    const gateways::BaseDeviceInfo& m_deviceInfo;
    const std::list<gateways::ItemInfo>& m_itemInfoList;

    const StateData* m_stateToRestore = nullptr;
};

}  // namespace scenes
