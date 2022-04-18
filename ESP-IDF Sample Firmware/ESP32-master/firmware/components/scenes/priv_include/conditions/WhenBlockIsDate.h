#pragma once

#include "WhenBlockIsDateBase.h"

namespace scenes {

class WhenBlockIsDate : public WhenBlockIsDateBase {
public:
    enum class DateMode { Daily, Weekly, Monthly, All };
    using Hours = TimeOfDaySet;

    WhenBlockIsDate() = default;

    WhenBlockIsDate( const Uuid& uuid,
                     const ParentType parentType,
                     const Uuid& parentUuid,
                     Weekdays&& weekdays,
                     Days&& days,
                     Hours&& hours )
    : WhenBlockIsDateBase( uuid, parentType, parentUuid, std::move( weekdays ), std::move( days ) )
    , m_hours( std::move( hours ) ) {}

    inline Type type() const override {
        return Type::WhenBlockIsDate;
    }

    DateMode dateMode() const;

    inline const auto& hours() const {
        return m_hours;
    }

    void accept( WhenBlockConstVisitor& visitor ) const override {
        visitor.visit( *this );
    }

private:
    bool writeImpl( common::IWriteStream& stream ) const override;

    bool readImpl( common::IReadStream& stream ) override;

private:
    Hours m_hours;
};

}  // end of namespace scenes
