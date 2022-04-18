#ifndef UNITCONVERTER_HPP_
#define UNITCONVERTER_HPP_

#include "cstdint"

namespace common {

class UnitConverter {
public:
    UnitConverter() = default;
    ~UnitConverter() = default;
    static float celsiusToFahrenheit( float celsius );
    static float fahrenheitToCelsius( float fahrenheit );
    static int32_t celsiusToFahrenheit( int32_t celsius, uint8_t precision );
    static int32_t fahrenheitToCelsius( int32_t fahrenheit, uint8_t precision );
    static uint32_t getPower( int index );

private:
    static const uint32_t power[7];
};

}  // namespace common

#endif /* UNITCONVERTER_HPP_ */
