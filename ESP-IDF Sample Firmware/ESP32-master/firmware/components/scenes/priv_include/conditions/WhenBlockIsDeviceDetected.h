#pragma once

#include "WhenBlockIsUnknowDeviceDetected.h"

namespace scenes {

class WhenBlockIsDeviceDetected : public WhenBlockIsUnknowDeviceDetected {
public:
    using DevicesIds = std::set<StringType>;

    using WhenBlockIsUnknowDeviceDetected::WhenBlockIsUnknowDeviceDetected;

    WhenBlockIsDeviceDetected( const Uuid& uuid,
                               const ParentType parentType,
                               const Uuid& parentUuid,
                               const int _minRssi,
                               const int _maxRssi,
                               DevicesIds&& devicesIds )
    : WhenBlockIsUnknowDeviceDetected( uuid, parentType, parentUuid, _minRssi, _maxRssi )
    , m_devicesIds( std::move( devicesIds ) ) {}

    inline Type type() const override {
        return Type::WhenBlockIsDeviceDetected;
    }

    inline void accept( WhenBlockConstVisitor& visitor ) const override {
        visitor.visit( *this );
    }

    inline const auto& devicesIds() const {
        return m_devicesIds;
    }

private:
    bool writeImpl( common::IWriteStream& stream ) const override {
        return WhenBlockIsUnknowDeviceDetected::writeImpl( stream ) && stream.write( m_devicesIds );
    }

    bool readImpl( common::IReadStream& stream ) override {
        return WhenBlockIsUnknowDeviceDetected::readImpl( stream ) && stream.read( m_devicesIds );
    }

private:
    DevicesIds m_devicesIds;
};

}  // namespace scenes
