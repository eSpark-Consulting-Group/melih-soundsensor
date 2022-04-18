#pragma once

#include "WhenBlock.h"

namespace scenes {

class WhenBlockInterval final : public WhenBlock {
public:
    enum class PeriodScale { Seconds, Minutes, Hours, Days };

    WhenBlockInterval() = default;

    WhenBlockInterval( const Uuid& uuid,
                       const ParentType parentType,
                       const Uuid& parentUuid,
                       const PeriodScale& periodScale,
                       const DelayTypeSec& period,
                       const TimePointType& creationTime );

    inline Type type() const override {
        return Type::WhenBlockInterval;
    }

    inline void accept( WhenBlockConstVisitor& visitor ) const override {
        visitor.visit( *this );
    }

    inline const auto& period() const {
        return m_period;
    }

    inline const auto& periodScale() const {
        return m_scale;
    }

    inline const auto& creationTime() const {
        return m_creationTime;
    }

private:
    bool writeImpl( common::IWriteStream& stream ) const override;

    bool readImpl( common::IReadStream& stream ) override;

private:
    PeriodScale m_scale;
    DelayTypeSec m_period;
    TimePointType m_creationTime;
};

}  // end of namespace scenes
