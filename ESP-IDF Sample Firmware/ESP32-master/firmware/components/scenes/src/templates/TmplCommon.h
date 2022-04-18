#pragma once

#include "CommonResources.h"
#include "JsonParserBase.h"
#include "IWriteStream.h"
#include "IReadStream.h"

#include <cstdint>
#include <tuple>

#include "DebugUtils.h"

namespace scenes {

using namespace common::resources;

using StringType = jssp::StringType;
using TokenType = StringType;
using TokensSet = std::set<TokenType>;
using TokensMap = std::map<TokenType, TokensSet>;

using BlockIndex = int;

class BlockCoord : public std::tuple<BlockIndex, BlockIndex> {
public:
    BlockCoord()
    : std::tuple<BlockIndex, BlockIndex>( -1, -1 ) {}

    BlockCoord( const BlockIndex from, const BlockIndex to )
    : std::tuple<BlockIndex, BlockIndex>( from, to ) {}

    bool isValid() const {
        return from() >= 0 && to() > from();
    }

    inline BlockIndex from() const {
        return std::get<0>( *this );
    }

    inline void setFrom( const BlockIndex from ) {
        std::get<0>( *this ) = from;
    }

    inline BlockIndex to() const {
        return std::get<1>( *this );
    }

    inline void setTo( const BlockIndex to ) {
        std::get<1>( *this ) = to;
    }

    inline void set( const BlockIndex from, const BlockIndex to ) {
        std::get<0>( *this ) = from;
        std::get<1>( *this ) = to;
    }

    inline bool isWithin( const BlockIndex pos ) const {
        return from() <= pos && to() >= pos;
    }

    inline void reset() {
        set( -1, -1 );
    }

    bool write( common::IWriteStream& stream ) const {
        return stream.write( from() ) && stream.write( to() );
    }

    bool read( common::IReadStream& stream ) {
        BlockIndex from, to;
        if( !stream.read( from ) || !stream.read( to ) ) {
            return false;
        }
        set( from, to );
        return true;
    }
};

class TemplateBlockData {
    BlockCoord m_coord, m_itemIdCoord, m_itemTokenCoord, m_dictBlockCoord;
    TokensSet m_tokens;
    TokensMap m_allowedDevices, m_prohibitedDevices;

public:
    bool isValid() const {
        return !m_tokens.empty() && m_coord.isValid() && m_itemIdCoord.isValid();
    }

    void reset() {
        m_coord.reset();
        m_itemIdCoord.reset();
        m_itemTokenCoord.reset();
        m_dictBlockCoord.reset();
        m_tokens.clear();
        m_allowedDevices.clear();
        m_prohibitedDevices.clear();
    }

    inline auto& blockCoord() {
        return m_coord;
    }

    inline const auto& blockCoord() const {
        return m_coord;
    }

    inline auto& itemIdCoord() {
        return m_itemIdCoord;
    }

    inline const auto& itemIdCoord() const {
        return m_itemIdCoord;
    }

    inline auto& itemTokenCoord() {
        return m_itemTokenCoord;
    }

    inline const auto& itemTokenCoord() const {
        return m_itemTokenCoord;
    }

    inline const auto& tokens() const {
        return m_tokens;
    }

    inline auto& tokens() {
        return m_tokens;
    }

    inline const auto& allowedDevices() const {
        return m_allowedDevices;
    }

    inline auto& allowedDevices() {
        return m_allowedDevices;
    }

    inline const auto& prohibitedDevices() const {
        return m_prohibitedDevices;
    }

    inline auto& prohibitedDevices() {
        return m_prohibitedDevices;
    }

    inline auto& dictBlockCoord() {
        return m_dictBlockCoord;
    }

    inline const auto& dictBlockCoord() const {
        return m_dictBlockCoord;
    }

    bool write( common::IWriteStream& stream ) const {
        return stream.write( m_coord ) && stream.write( m_itemIdCoord ) && stream.write( m_itemTokenCoord ) &&
               stream.write( m_dictBlockCoord ) && stream.write( m_tokens ) && stream.write( m_allowedDevices ) &&
               stream.write( m_prohibitedDevices );
    }

    bool read( common::IReadStream& stream ) {
        return stream.read( m_coord ) && stream.read( m_itemIdCoord ) && stream.read( m_itemTokenCoord ) &&
               stream.read( m_dictBlockCoord ) && stream.read( m_tokens ) && stream.read( m_allowedDevices ) &&
               stream.read( m_prohibitedDevices );
    }
};

}  // namespace scenes
