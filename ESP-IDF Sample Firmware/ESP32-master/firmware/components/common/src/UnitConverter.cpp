#include <UnitConverter.h>

namespace common {

float UnitConverter::celsiusToFahrenheit( float celsius ) {
    return celsius * 9 / 5 + 32;
}

float UnitConverter::fahrenheitToCelsius( float fahrenheit ) {
    return ( fahrenheit - 32 ) * 5 / 9;
}

int32_t UnitConverter::celsiusToFahrenheit( int32_t celsius, uint8_t precision ) {
    return ( (int64_t)celsius * 9 ) / 5 + ( 32 * power[precision] );
}

int32_t UnitConverter::fahrenheitToCelsius( int32_t fahrenheit, uint8_t precision ) {
    return ( ( (int64_t)fahrenheit - ( 32 * power[precision] ) ) * 5 ) / 9;
}

const uint32_t UnitConverter::power[7] = { 1, 10, 100, 1000, 10000, 100000, 1000000 };

uint32_t UnitConverter::getPower( int index ) {
    if( 0 <= index && index < 7 ) {
        return power[index];
    }
    return 1;
}

}  // namespace common
