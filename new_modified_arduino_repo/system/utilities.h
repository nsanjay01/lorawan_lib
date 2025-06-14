/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: Helper functions implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#ifndef __UTILITIES_H__
#define __UTILITIES_H__

// #include <Arduino.h>
#include <stdint.h>
#include "sx126x-debug.h"
#include <stdlib.h>

/*!
 * \brief Returns 2 raised to the power of n
 *
 * \param  n power value
 * \retval result of raising 2 to the power n
 */
#define POW2(n) (1 << n)

/*!
 * \brief Initializes the pseudo random generator initial value
 *
 * \param  seed Pseudo random generator initial value
 */
void srand1(uint32_t seed);

/*!
 * \brief Computes a random number between min and max
 *
 * \param  min range minimum value
 * \param  max range maximum value
 * \retval random random value in range min..max
 */
int32_t randr(int32_t min, int32_t max);

/*!
 * \brief Copies size elements of src array to dst array
 *
 * \remark STM32 Standard memcpy function only works on pointers that are aligned
 *
 * \param  dst  Destination array
 * \param   src  Source array
 * \param   size Number of bytes to be copied
 */
void memcpy1(uint8_t *dst, const uint8_t *src, uint16_t size);

/*!
 * \brief Copies size elements of src array to dst array reversing the byte order
 *
 * \param  dst  Destination array
 * \param   src  Source array
 * \param   size Number of bytes to be copied
 */
void memcpyr(uint8_t *dst, const uint8_t *src, uint16_t size);

/*!
 * \brief Set size elements of dst array with value
 *
 * \remark STM32 Standard memset function only works on pointers that are aligned
 *
 * \param  dst   Destination array
 * \param   value Default value
 * \param   size  Number of bytes to be copied
 */
void memset1(uint8_t *dst, uint8_t value, uint16_t size);

/*!
 * \brief Converts a nibble to an hexadecimal character
 *
 * \param  a   Nibble to be converted
 * \retval hexChar Converted hexadecimal character
 */
int8_t Nibble2HexChar(uint8_t a);

/** Leaves the minimum of the two 32-bit arguments */
/*lint -emacro(506, MIN) */ /* Suppress "Constant value Boolean */
#define T_MIN(a, b) ((a) < (b) ? (a) : (b))
/** Leaves the maximum of the two 32-bit arguments */
/*lint -emacro(506, T_MAX) */ /* Suppress "Constant value Boolean */
#define T_MAX(a, b) ((a) < (b) ? (b) : (a))

#endif // __UTILITIES_H__
