#pragma once

#include <vector>
#include <znet/znet.h>
#include <IValue.h>

namespace common {

class RgbValue final : public IValue {
public:
    using UPtr = std::unique_ptr<RgbValue>;

    enum class Color : uint8_t {
        Unknown,

        Wwhite,
        Cwhite,
        Red,
        Green,
        Blue,
        Amber,
        Cyan,
        Purple,

        Indexed
    };

    struct ColorComponent {
        Color color;
        uint8_t value;

        ColorComponent( Color _color, uint8_t _value )
        : color( _color )
        , value( _value ) {}

        bool operator==( const ColorComponent& component ) const {
            return color == component.color && value == component.value;
        }
        bool operator>( const ColorComponent& component ) const {
            return color == component.color && value > component.value;
        }
        bool operator<( const ColorComponent& component ) const {
            return color == component.color && value < component.value;
        }
    };

    using RgbData = std::vector<ColorComponent>;
    using Iterator = RgbData::iterator;
    using ConstIterator = RgbData::const_iterator;

public:
    static const char* componentIdToStr( RgbValue::Color color );
    static RgbValue::Color strToColorComponentId( const char* str );

public:
    RgbValue();

    RgbValue( RgbData&& components );

    RgbValue( const RgbValue& ) = default;

    void accept( IValueVisitorConst& visitor ) const override;

    void accept( IValueVisitor& visitor ) override;

    size_t getComponentsCount() const;

    const RgbData& get() const;

    RgbData& get();

    Iterator findComponent( Color color );
    ConstIterator findComponent( Color color ) const;
    bool hasComponent( Color color ) const;
    uint8_t getComponentValue( Color color ) const;

    void set( RgbData&& components );
    void setComponentValue( Color color, uint8_t value );

    bool operator>( const RgbValue& val ) const;
    bool operator<( const RgbValue& val ) const;
    bool operator==( const RgbValue& val ) const;

private:
    int matchCount( const RgbValue& val ) const;

    RgbData m_components;
};

inline RgbValue::Color& operator++( RgbValue::Color& val ) {
    reinterpret_cast<uint8_t&>( val ) += 1;
    return val;
}

}  // namespace common
