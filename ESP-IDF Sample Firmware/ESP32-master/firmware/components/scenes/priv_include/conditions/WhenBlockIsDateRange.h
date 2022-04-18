#pragma once

#include "WhenBlock.h"

namespace scenes {

class WhenBlockIsDateRange : public WhenBlock {
public:
    WhenBlockIsDateRange() = default;

    WhenBlockIsDateRange( const Uuid& uuid,
                          const ParentType parentType,
                          const Uuid& parentUuid,
                          TimeOfDay&& startTime,
                          TimeOfDay&& endTime,
                          DayOfMonth&& startDay,
                          DayOfMonth&& endDay,
                          Month&& startMonth,
                          Month&& endMonth,
                          Year&& startYear,
                          Year&& endYear )
    : WhenBlock( uuid, parentType, parentUuid )
    , m_startTime( std::move( startTime ) )
    , m_endTime( std::move( endTime ) )
    , m_startDay( std::move( startDay ) )
    , m_endDay( std::move( endDay ) )
    , m_startMonth( std::move( startMonth ) )
    , m_endMonth( std::move( endMonth ) )
    , m_startYear( startYear )
    , m_endYear( endYear ) {}

    inline Type type() const override {
        return Type::WhenBlockIsDateRange;
    }

    inline auto& startTime() {
        return m_startTime;
    }

    inline const auto& startTime() const {
        return m_startTime;
    }

    inline auto& endTime() {
        return m_endTime;
    }

    inline const auto& endTime() const {
        return m_endTime;
    }

    inline auto& startDay() {
        return m_startDay;
    }

    inline const auto& startDay() const {
        return m_startDay;
    }

    inline auto& endDay() {
        return m_endDay;
    }

    inline const auto& endDay() const {
        return m_endDay;
    }

    inline auto& startMonth() {
        return m_startMonth;
    }

    inline const auto& startMonth() const {
        return m_startMonth;
    }

    inline auto& endMonth() {
        return m_endMonth;
    }

    inline const auto& endMonth() const {
        return m_endMonth;
    }

    inline auto& startYear() {
        return m_startYear;
    }

    inline const auto& startYear() const {
        return m_startYear;
    }

    inline auto& endYear() {
        return m_endYear;
    }

    inline const auto& endYear() const {
        return m_endYear;
    }

    void accept( WhenBlockConstVisitor& visitor ) const override {
        visitor.visit( *this );
    }

private:
    bool writeImpl( common::IWriteStream& stream ) const override {
        return stream.write( m_startTime ) && stream.write( m_endTime ) && stream.write( m_startDay ) &&
               stream.write( m_endDay ) && stream.write( m_startMonth ) && stream.write( m_endMonth ) &&
               stream.write( m_startYear ) && stream.write( m_endYear );
    }

    bool readImpl( common::IReadStream& stream ) override {
        return stream.read( m_startTime ) && stream.read( m_endTime ) && stream.read( m_startDay ) &&
               stream.read( m_endDay ) && stream.read( m_startMonth ) && stream.read( m_endMonth ) &&
               stream.read( m_startYear ) && stream.read( m_endYear );
    }

private:
    TimeOfDay m_startTime, m_endTime;
    DayOfMonth m_startDay, m_endDay;
    Month m_startMonth, m_endMonth;
    Year m_startYear, m_endYear;
};

}  // namespace scenes
