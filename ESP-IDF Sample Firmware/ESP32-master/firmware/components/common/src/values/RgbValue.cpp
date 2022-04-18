#include <values/RgbValue.h>
#include <algorithm>
#include <cstring>
#include <znet/znet.hpp>
#include <IValueVisitor.h>

bool operator==( const znet_cl_component_t& val1, const znet_cl_component_t& val2 ) {
    return val1.component_id == val2.component_id && val1.value == val2.value;
}

namespace common {

const char* RgbValue::componentIdToStr( RgbValue::Color color ) {
    switch( color ) {
    case RgbValue::Color::Wwhite:
        return "wwhite";
    case RgbValue::Color::Cwhite:
        return "cwhite";
    case RgbValue::Color::Red:
        return "red";
    case RgbValue::Color::Green:
        return "green";
    case RgbValue::Color::Blue:
        return "blue";
    case RgbValue::Color::Amber:
        return "amber";
    case RgbValue::Color::Cyan:
        return "cyan";
    case RgbValue::Color::Purple:
        return "purple";
    case RgbValue::Color::Indexed:
        return "indexed";
    default:
        return nullptr;
    }
}

RgbValue::Color RgbValue::strToColorComponentId( const char* str ) {
    if( strcmp( str, "wwhite" ) == 0 ) {
        return RgbValue::Color::Wwhite;
    }
    else if( strcmp( str, "cwhite" ) == 0 ) {
        return RgbValue::Color::Cwhite;
    }
    else if( strcmp( str, "red" ) == 0 ) {
        return RgbValue::Color::Red;
    }
    else if( strcmp( str, "green" ) == 0 ) {
        return RgbValue::Color::Green;
    }
    else if( strcmp( str, "blue" ) == 0 ) {
        return RgbValue::Color::Blue;
    }
    else if( strcmp( str, "amber" ) == 0 ) {
        return RgbValue::Color::Amber;
    }
    else if( strcmp( str, "cyan" ) == 0 ) {
        return RgbValue::Color::Cyan;
    }
    else if( strcmp( str, "purple" ) == 0 ) {
        return RgbValue::Color::Purple;
    }
    else if( strcmp( str, "indexed" ) == 0 ) {
        return RgbValue::Color::Indexed;
    }

    return RgbValue::Color::Unknown;
}

RgbValue::RgbValue() {
    m_components.reserve( 8 );  // most bulbs have 5 colors, some could have 8 colors
}

RgbValue::RgbValue( RgbData&& components )
: m_components( std::move( components ) ) {}

void RgbValue::accept( IValueVisitorConst& visitor ) const {
    visitor.visit( *this );
}

void RgbValue::accept( IValueVisitor& visitor ) {
    visitor.visit( *this );
}

size_t RgbValue::getComponentsCount() const {
    return m_components.size();
}

const RgbValue::RgbData& RgbValue::get() const {
    return m_components;
}

RgbValue::RgbData& RgbValue::get() {
    return m_components;
}

RgbValue::Iterator RgbValue::findComponent( RgbValue::Color color ) {
    return std::find_if( std::begin( m_components ),
                         std::end( m_components ),
                         [color]( const RgbValue::ColorComponent& component ) { return component.color == color; } );
}

RgbValue::ConstIterator RgbValue::findComponent( RgbValue::Color color ) const {
    return std::find_if( std::begin( m_components ),
                         std::end( m_components ),
                         [color]( const RgbValue::ColorComponent& component ) { return component.color == color; } );
}

bool RgbValue::hasComponent( RgbValue::Color color ) const {
    return findComponent( color ) != std::end( m_components );
}

uint8_t RgbValue::getComponentValue( RgbValue::Color color ) const {
    auto it = findComponent( color );
    if( it != std::end( m_components ) ) {
        return it->value;
    }
    else {
        return 0;
    }
}

void RgbValue::set( RgbData&& components ) {
    m_components = std::move( components );
}

void RgbValue::setComponentValue( RgbValue::Color color, uint8_t value ) {
    auto component = findComponent( color );
    if( component != std::end( m_components ) ) {
        component->value = value;
    }
    else {
        m_components.push_back( RgbValue::ColorComponent{ color, value } );
    }
}

bool RgbValue::operator>( const RgbValue& val ) const {
    return matchCount( val ) > 0;
}

bool RgbValue::operator<( const RgbValue& val ) const {
    return matchCount( val ) < 0;
}

bool RgbValue::operator==( const RgbValue& val ) const {
    return matchCount( val ) == 0;
}

int RgbValue::matchCount( const RgbValue& val ) const {
    int counter = 0;

    for( auto component : m_components ) {
        if( val.hasComponent( component.color ) ) {
            if( component.value > val.getComponentValue( component.color ) ) {
                counter++;
            }
            else if( component.value < val.getComponentValue( component.color ) ) {
                counter--;
            }
        }
        else {
            counter++;
        }
    }

    auto it = val.get().begin();
    for( ; it != val.get().end(); ++it ) {
        if( !hasComponent( ( *it ).color ) ) {
            counter--;
        }
    }

    return counter;
}

}  // namespace common
