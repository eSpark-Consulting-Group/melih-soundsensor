#pragma once

#include "WhenBlock.h"

namespace scenes {

class WhenBlockIsOnce final : public WhenBlock {
public:
    using DateType = unsigned int;

    WhenBlockIsOnce() = default;

    WhenBlockIsOnce( const Uuid& uuid,
                     const ParentType parentType,
                     const Uuid& parentUuid,
                     const DateType day,
                     const DateType month,
                     const DateType year,
                     TimeOfDay&& timeOfDay )
    : WhenBlock( uuid, parentType, parentUuid )
    , m_day( day )
    , m_month( month )
    , m_year( year )
    , m_timeOfDay( std::move( timeOfDay ) ) {}

    inline auto day() const {
        return m_day;
    }

    inline auto month() const {
        return m_month;
    }

    inline auto year() const {
        return m_year;
    }

    inline auto dayTime() const {
        return m_timeOfDay;
    }

    inline Type type() const override {
        return Type::WhenBlockIsOnce;
    }

    inline void accept( WhenBlockConstVisitor& visitor ) const override {
        visitor.visit( *this );
    }

private:
    bool writeImpl( common::IWriteStream& stream ) const override {
        return stream.write( m_day ) && stream.write( m_month ) && stream.write( m_year ) &&
               stream.write( m_timeOfDay );
    }

    bool readImpl( common::IReadStream& stream ) override {
        return stream.read( m_day ) && stream.read( m_month ) && stream.read( m_year ) && stream.read( m_timeOfDay );
    }

private:
    DateType m_day = 0, m_month = 0, m_year = 0;
    TimeOfDay m_timeOfDay;
};

}  // end of namespace scenes
