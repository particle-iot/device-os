/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "configuration.h"

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#if (uECC_WORD_SIZE == 1)

#define uECC_WORDS 28
#define uECC_N_WORDS 28

#define Curve_P {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                   0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, \
                   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, \
                   0xFF, 0xFF, 0xFF, 0xFF}
#define Curve_B {0xB4, 0xFF, 0x55, 0x23, 0x43, 0x39, 0x0B, 0x27, \
                   0xBA, 0xD8, 0xBF, 0xD7, 0xB7, 0xB0, 0x44, 0x50, \
                   0x56, 0x32, 0x41, 0xF5, 0xAB, 0xB3, 0x04, 0x0C, \
                   0x85, 0x0A, 0x05, 0xB4}
#define Curve_G { \
    {0x21, 0x1D, 0x5C, 0x11, 0xD6, 0x80, 0x32, 0x34, \
        0x22, 0x11, 0xC2, 0x56, 0xD3, 0xC1, 0x03, 0x4A, \
        0xB9, 0x90, 0x13, 0x32, 0x7F, 0xBF, 0xB4, 0x6B, \
        0xBD, 0x0C, 0x0E, 0xB7}, \
    {0x34, 0x7E, 0x00, 0x85, 0x99, 0x81, 0xD5, 0x44, \
        0x64, 0x47, 0x07, 0x5A, 0xA0, 0x75, 0x43, 0xCD, \
        0xE6, 0xDF, 0x22, 0x4C, 0xFB, 0x23, 0xF7, 0xB5, \
        0x88, 0x63, 0x37, 0xBD}}
#define Curve_N {0x3D, 0x2A, 0x5C, 0x5C, 0x45, 0x29, 0xDD, 0x13, \
                   0x3E, 0xF0, 0xB8, 0xE0, 0xA2, 0x16, 0xFF, 0xFF, \
                   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, \
                   0xFF, 0xFF, 0xFF, 0xFF}

#elif (uECC_WORD_SIZE == 4)

#define uECC_WORDS 7
#define uECC_N_WORDS 7

#define Curve_P {0x00000001, 0x00000000, 0x00000000, 0xFFFFFFFF, \
                   0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF}
#define Curve_B {0x2355FFB4, 0x270B3943, 0xD7BFD8BA, 0x5044B0B7, \
                   0xF5413256, 0x0C04B3AB, 0xB4050A85}
#define Curve_G { \
    {0x115C1D21, 0x343280D6, 0x56C21122, 0x4A03C1D3, \
     0x321390B9, 0x6BB4BF7F, 0xB70E0CBD}, \
    {0x85007E34, 0x44D58199, 0x5A074764, 0xCD4375A0, \
     0x4C22DFE6, 0xB5F723FB, 0xBD376388}}
#define Curve_N {0x5C5C2A3D, 0x13DD2945, 0xE0B8F03E, 0xFFFF16A2, \
                   0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF}

#elif (uECC_WORD_SIZE == 8)

#define uECC_WORDS 4
#define uECC_N_WORDS 4

#define Curve_P {0x0000000000000001ull, 0xFFFFFFFF00000000ull, \
                   0xFFFFFFFFFFFFFFFFull, 0x00000000FFFFFFFFull}
#define Curve_B {0x270B39432355FFB4ull, 0x5044B0B7D7BFD8BAull, \
                   0x0C04B3ABF5413256ull, 0x00000000B4050A85ull}
#define Curve_G { \
    {0x343280D6115C1D21ull, 0x4A03C1D356C21122ull, 0x6BB4BF7F321390B9ull, 0x00000000B70E0CBDull}, \
    {0x44D5819985007E34ull, 0xCD4375A05A074764ull, 0xB5F723FB4C22DFE6ull, 0x00000000BD376388ull}}
#define Curve_N {0x13DD29455C5C2A3Dull, 0xFFFF16A2E0B8F03Eull, \
                   0xFFFFFFFFFFFFFFFFull, 0x00000000FFFFFFFFull}

#endif /* (uECC_WORD_SIZE == 8) */


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
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/


#ifdef __cplusplus
} /* extern "C" */
#endif
