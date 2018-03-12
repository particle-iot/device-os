/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/*
 * Small footprint formatted print directly to stdout
 *
 * NOTES:
 *  These values are upgraded to 32-bit values when variable args are used.
 *      char        unsigned char
 *      int8_t      uint8_t
 *      int16_t     uint16_t
 *
 * Uses:
 *    platform_stdio_write()
 *    va_start(), va_arg(), va_stop()
 *    strlen()
 *    memset(), memmove().
 *
 * Supports:
 *
 *  %%      - Prints the '%' character
 *
 *  %d      - decimal number
 *              supports negative
 *              ignores 'l' modifier (all values upgraded to longs)
 *              supports leading spaces and zeros
 *
 *                  mini_printf("%d %2d %04d %ld\r\n", num1, num2, num3, num4);
 *
 *  %x      - hexadecimal number
 *              ignores 'l' modifier (all values upgraded to longs)
 *              supports leading spaces and zeros
 *
 *                  mini_printf("0x%x 0x%2x 0x%04x 0x%lx\r\n", num1, num2, num3, num4);
 *
 *  %c      - single character
 *
 *                  mini_printf("%c %c%c\r\n", 'W', 'H', 'i');
 *
 *  %s      - character string
 *
 *                  mini_printf("%s %s", my_string, "YeeHaw\r\n");
 *
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Static Function Declarations
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

int mini_printf( const char* format, ...);
int hex_dump_print(const void* data, uint16_t length, int show_ascii);

#ifdef __cplusplus
} /* extern "C" */
#endif
