/*
 * Copyright (c) 2015 Broadcom
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * 3. Neither the name of Broadcom nor the names of other contributors to this
 * software may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * 4. This software may not be used as a standalone product, and may only be used as
 * incorporated in your product or device that incorporates Broadcom wireless connectivity
 * products and solely for the purpose of enabling the functionalities of such Broadcom products.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY WARRANTIES OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT, ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @file
 * Defines platform constants
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                    Macros
 ******************************************************/

#ifndef  TO_STRING
#define TO_STRING( a ) #a
#endif

#ifndef RESULT_ENUM
#define RESULT_ENUM( prefix, name, value )  prefix ## name = (value)
#endif /* ifndef RESULT_ENUM */

/* These Enum result values are for platform errors
 * Values: 1000 - 1999
 */
#define PLATFORM_RESULT_LIST( prefix ) \
        RESULT_ENUM( prefix, SUCCESS,                          0 ),   /**< Success */               \
        RESULT_ENUM( prefix, PENDING,                          1 ),   /**< Pending */               \
        RESULT_ENUM( prefix, TIMEOUT,                          2 ),   /**< Timeout */               \
        RESULT_ENUM( prefix, PARTIAL_RESULTS,                  3 ),   /**< Partial results */       \
        RESULT_ENUM( prefix, ERROR,                            4 ),   /**< Error */                 \
        RESULT_ENUM( prefix, BADARG,                           5 ),   /**< Bad Arguments */         \
        RESULT_ENUM( prefix, BADOPTION,                        6 ),   /**< Mode not supported */    \
        RESULT_ENUM( prefix, UNSUPPORTED,                      7 ),   /**< Unsupported function */  \
        RESULT_ENUM( prefix, UNINITLIASED,                  6008 ),   /**< Unitialised */           \
        RESULT_ENUM( prefix, INIT_FAIL,                     6009 ),   /**< Initialisation failed */ \
        RESULT_ENUM( prefix, NO_EFFECT,                     6010 ),   /**< No effect */             \
        RESULT_ENUM( prefix, FEATURE_DISABLED,              6011 ),   /**< Feature disabled */      \
        RESULT_ENUM( prefix, NO_WLAN_POWER,                 6012 ),   /**< WLAN core is not powered */ \
        RESULT_ENUM( prefix, SPI_SLAVE_INVALID_COMMAND,     6013 ),   /**< Command is invalid */ \
        RESULT_ENUM( prefix, SPI_SLAVE_ADDRESS_UNAVAILABLE, 6014 ),   /**< Address specified in the command is unavailable */ \
        RESULT_ENUM( prefix, SPI_SLAVE_LENGTH_MISMATCH,     6015 ),   /**< Length specified in the command doesn't match with the actual data length */ \
        RESULT_ENUM( prefix, SPI_SLAVE_READ_NOT_ALLOWED,    6016 ),   /**< Read operation is not allowed for the address specified */ \
        RESULT_ENUM( prefix, SPI_SLAVE_WRITE_NOT_ALLOWED,   6017 ),   /**< Write operation is not allowed for the address specified */ \
        RESULT_ENUM( prefix, SPI_SLAVE_HARDWARE_ERROR,      6018 ),   /**< Hardware error occurred during transfer */

/******************************************************
 *                   Enumerations
 ******************************************************/

/* Platform result enumeration */
typedef enum
{
    PLATFORM_RESULT_LIST( PLATFORM_ )
} platform_result_t;

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

