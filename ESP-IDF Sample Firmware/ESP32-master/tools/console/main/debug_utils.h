/*
 * debug_utils.h
 *
 *  Created on: Jan 18, 2018
 *      Author: boil
 */

#ifndef MAIN_DEBUG_UTILS_H_
#define MAIN_DEBUG_UTILS_H_

#include <stdio.h>

static void mlog_uint32(const char* file, const char* func, int line, const char* name, uint32_t value)
{
    printf("%s:%s:%d %s=%02x:%02x:%02x:%02x\n",
            file,
            func,
            line,
            name,
            (value & 0xFF000000) >> 24, (value & 0x00FF0000) >> 16, (value & 0x0000FF00) >> 8, value & 0x000000FF);
}

//static void mlog_uint8(const char* file, const char* func, int line, const char* name, uint8_t value)
//{
//    printf("%s:%s:%d %s=%02x\n",
//            file,
//            func,
//            line,
//            name,
//            value);
//}

#if defined(LOG_XX)

#error the redefinition of LOG_XX macro detected

#else

#define LOG_XX(func, value) \
	do { func(__FILE__, __FUNCTION__, __LINE__, #value, (value)); } while (0);

#define LOG_U32(value) \
		LOG_XX(mlog_uint32, value)

#define LOG_U0(value) \
		LOG_XX(mlog_uint8, value)

#endif
#endif /* MAIN_DEBUG_UTILS_H_ */
