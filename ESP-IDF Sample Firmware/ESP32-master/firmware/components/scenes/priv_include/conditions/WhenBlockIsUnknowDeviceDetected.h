#pragma once

#include "WhenBlock.h"

namespace scenes {

class WhenBlockIsUnknowDeviceDetected : public WhenBlock {
public:
    using RssiType = int;

    WhenBlockIsUnknowDeviceDetected()
    : m_minRssi{ 0 }
    , m_maxRssi{ 0 } {};

    WhenBlockIsUnknowDeviceDetected( const Uuid& uuid,
                                     const ParentType parentType,
                                     const Uuid& parentUuid,
                                     const int _minRssi,
                                     const int _maxRssi )
    : WhenBlock( uuid, parentType, parentUuid )
    , m_minRssi( _minRssi )
    , m_maxRssi( _maxRssi ) {}

    inline Type type() const override {
        return Type::WhenBlockIsUnknowDeviceDetected;
    }

    inline void accept( WhenBlockConstVisitor& visitor ) const override {
        visitor.visit( *this );
    }

    inline auto minRssi() const {
        return m_minRssi;
    }

    inline auto maxRssi() const {
        return m_maxRssi;
    }

    inline bool checkRssi( const RssiType rssi ) const {
        return m_minRssi <= rssi && rssi <= m_maxRssi;
    }

protected:
    bool writeImpl( common::IWriteStream& stream ) const override {
        return stream.write( m_minRssi ) && stream.write( m_maxRssi );
    }

    bool readImpl( common::IReadStream& stream ) override {
        return stream.read( m_minRssi ) && stream.read( m_maxRssi );
    }

private:
    RssiType m_minRssi, m_maxRssi;
};

}  // namespace scenes
