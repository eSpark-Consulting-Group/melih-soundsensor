#pragma once

#include "WhenBlock.h"

namespace scenes {

class WhenBlockIsDateBase : public WhenBlock {
public:
    using Days = Uint8Set;
    using Weekdays = Uint8Set;

    using WhenBlock::WhenBlock;

    WhenBlockIsDateBase( const Uuid& uuid,
                         const ParentType parentType,
                         const Uuid& parentUuid,
                         Weekdays&& weekdays,
                         Days&& days )
    : WhenBlock( uuid, parentType, parentUuid )
    , m_weekdays( std::move( weekdays ) )
    , m_days( std::move( days ) ) {}

    inline const auto& weekdays() const {
        return m_weekdays;
    }

    inline bool hasWeekdays() const {
        return !weekdays().empty();
    }

    inline const auto& days() const {
        return m_days;
    }

    inline bool hasDays() const {
        return !days().empty();
    }

    bool isDayOfWeek( const TimePointType& tp ) const;

    bool isDayOfMonth( const TimePointType& tp ) const;

protected:
    bool writeImpl( common::IWriteStream& stream ) const override;

    bool readImpl( common::IReadStream& stream ) override;

private:
    Weekdays m_weekdays;
    Days m_days;
};

}  // end of namespace scenes
