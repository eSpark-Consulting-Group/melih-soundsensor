#pragma once

#include "WhenBlockIsDateBase.h"

namespace scenes {

class WhenBlockIsSunState final : public WhenBlockIsDateBase {
public:
    enum class SunMode { Sunrise, Sunset, All };

    WhenBlockIsSunState() = default;

    WhenBlockIsSunState( const Uuid& uuid,
                         const ParentType parentType,
                         const Uuid& parentUuid,
                         Weekdays&& weekdays,
                         Days&& days,
                         const SunMode sunMode,
                         const TimeInterval& interval )
    : WhenBlockIsDateBase( uuid, parentType, parentUuid, std::move( weekdays ), std::move( days ) )
    , m_sunMode( sunMode )
    , m_timeInterval( interval ) {}

    inline Type type() const override {
        return Type::WhenBlockIsSunState;
    }

    inline SunMode sunMode() const {
        return m_sunMode;
    }

    inline const auto& timeInterval() const {
        return m_timeInterval;
    }

    TimeOfDay trigTime( const TimeInfo& timeInfo ) const;

    void accept( WhenBlockConstVisitor& visitor ) const override {
        visitor.visit( *this );
    }

private:
    bool writeImpl( common::IWriteStream& stream ) const override;

    bool readImpl( common::IReadStream& stream ) override;

private:
    SunMode m_sunMode = SunMode::Sunrise;
    TimeInterval m_timeInterval;
};

}  // end of namespace scenes
