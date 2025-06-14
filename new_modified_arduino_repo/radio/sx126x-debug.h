/**
 * @file sx126x-debug.h
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief Unified debug output for all platforms
 * 		Set LIB_DEBUG to 1 to enable debug output
 *      - either here in this header files (Arduino IDE)
 *      - or globale with build_flags = -DLIB_DEBUG=1 in platformio.ini (PIO)
 * @version 0.1
 * @date 2021-04-04
 * 
 * @copyright Copyright (c) 2021
 * 
 */


#ifndef __SX126XDEBGUG__
#define __SX126XDEBGUG__


#include <stdio.h>

// If not on PIO or not defined in platformio.ini
#ifndef LIB_DEBUG
// Debug output set to 0 to disable app debug output
#define LIB_DEBUG 1
#endif

#if LIB_DEBUG > 0
#define LOG_LIB(tag, ...)                \
	do                                   \
	{                                    \
		if (tag)                         \
			printf("<%s> ", tag);        \
		    printf(__VA_ARGS__);         \
		    printf("\n");                \
	} while (0)
#else
#define LOG_LIB(...)
#endif

#endif

