#pragma once

#include "values/IntValue.h"
#include "values/TableValue.h"

#include "BusCommand.h"
#include "BusCommandHandleEvent.h"

#include <CommonResources.h>

/**
 * Bus commands defined in this file cover functionality of ScheduleEntryLock ZNet Command Class.
 */

namespace bus {
namespace command {

class EntryLockSchedule {
public:
    EntryLockSchedule( uint8_t nodeId, uint8_t channelId, uint8_t userId, uint8_t slotId )
    : m_nodeId( nodeId )
    , m_channelId( channelId )
    , m_userId( userId )
    , m_slotId( slotId ) {}

    uint8_t nodeId() const {
        return m_nodeId;
    }

    uint8_t channelId() const {
        return m_channelId;
    }

    uint8_t userId() const {
        return m_userId;
    }

    uint8_t slotId() const {
        return m_slotId;
    }

private:
    uint8_t m_nodeId;
    uint8_t m_channelId;
    uint8_t m_userId;
    uint8_t m_slotId;
};

class SetEntryLockSchedule : public EntryLockSchedule {
public:
    SetEntryLockSchedule( uint8_t nodeId,
                          uint8_t channelId,
                          uint8_t userId,
                          uint8_t slotId,
                          std::unique_ptr<::common::TableValue> value )
    : EntryLockSchedule( nodeId, channelId, userId, slotId )
    , m_value( std::move( value ) ) {}

    const ::common::TableValue& value() const {
        return *m_value;
    }

private:
    std::unique_ptr<::common::TableValue> m_value;
};

class SetDailyRepeatingEntryLockSchedule : public SetEntryLockSchedule, public IBusCommand {
public:
    using SetEntryLockSchedule::SetEntryLockSchedule;
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

class SetWeekDayEntryLockSchedule : public SetEntryLockSchedule, public IBusCommand {
public:
    using SetEntryLockSchedule::SetEntryLockSchedule;
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

class SetYearDayEntryLockSchedule : public SetEntryLockSchedule, public IBusCommand {
public:
    using SetEntryLockSchedule::SetEntryLockSchedule;
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

class RemoveDailyRepeatingEntryLockSchedule : public EntryLockSchedule, public IBusCommand {
public:
    using EntryLockSchedule::EntryLockSchedule;
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

class RemoveWeekDayEntryLockSchedule : public EntryLockSchedule, public IBusCommand {
public:
    using EntryLockSchedule::EntryLockSchedule;
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

class RemoveYearDayEntryLockSchedule : public EntryLockSchedule, public IBusCommand {
public:
    using EntryLockSchedule::EntryLockSchedule;
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

class RequestDailyRepeatingEntryLockSchedule : public EntryLockSchedule, public IBusCommand {
public:
    using EntryLockSchedule::EntryLockSchedule;
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

class RequestWeekDayEntryLockSchedule : public EntryLockSchedule, public IBusCommand {
public:
    using EntryLockSchedule::EntryLockSchedule;
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

class RequestYearDayEntryLockSchedule : public EntryLockSchedule, public IBusCommand {
public:
    using EntryLockSchedule::EntryLockSchedule;
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

}  // namespace command
}  // namespace bus
