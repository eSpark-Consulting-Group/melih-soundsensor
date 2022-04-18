#pragma once
// took from https://github.com/antirez/redis/blob/95b1979c321eb6353f75df892ab8be68cf8f9a77/src/crc16.c

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t calcCrc16( const char* buf, int len, uint16_t _crc );

#ifdef __cplusplus
}
#endif
