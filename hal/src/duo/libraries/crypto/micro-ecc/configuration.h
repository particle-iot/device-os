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

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/* Curve selection options. */
#define uECC_secp160r1     1
#define uECC_secp192r1     2
#define uECC_secp256r1     3
#define uECC_secp256k1     4
#define uECC_secp224r1     5

/* Inline assembly options.
uECC_asm_none  - Use standard C99 only.
uECC_asm_small - Use GCC inline assembly for the target platform (if available), optimized for
                 minimum size.
uECC_asm_fast  - Use GCC inline assembly optimized for maximum speed. */
#define uECC_asm_none      0
#define uECC_asm_small     1
#define uECC_asm_fast      2


/* uECC_SQUARE_FUNC - If enabled (defined as nonzero), this will cause a specific function to be
used for (scalar) squaring instead of the generic multiplication function. This will make things
faster by about 8% but increases the code size. */

/* Platform selection options.
If uECC_PLATFORM is not defined, the code will try to guess it based on compiler macros.
Possible values for uECC_PLATFORM are defined below: */
#define uECC_arch_other     0
#define uECC_x86            1
#define uECC_x86_64         2
#define uECC_arm            3
#define uECC_arm_thumb      4
#define uECC_avr            5
#define uECC_arm_thumb2     6

#define uECC_secp160r1_size     20
#define uECC_secp192r1_size     24
#define uECC_secp224r1_size     28
#define uECC_secp256k1_size     32
#define uECC_secp256r1_size     32


/* Selected curve */
#define uECC_CURVE   uECC_secp256r1
#define uECC_BYTES   uECC_secp256r1_size

/* Selected assembly type */
#define uECC_ASM     uECC_asm_fast

/* Enable square function by default */
#define uECC_SQUARE_FUNC 1

/* Set the platform */  /* see http://sourceforge.net/p/predef/wiki/Architectures/ */
#if defined ( __x86_64__ )  || defined ( __i386__ ) || defined ( _M_X64 ) || defined ( _M_IX86 )
#define uECC_PLATFORM   uECC_x86
#else
#define uECC_PLATFORM   uECC_arm_thumb2
#endif


#define uECC_WORD_SIZE    4

#include "secp256r1_constants.h"

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
