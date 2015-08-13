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

#define TCPIP_RESULT_LIST( prefix ) \
        RESULT_ENUM( prefix, SUCCESS,                 0 ),   /**< Success */               \
        RESULT_ENUM( prefix, PENDING,                 1 ),   /**< Pending */               \
        RESULT_ENUM( prefix, TIMEOUT,                 2 ),   /**< Timeout */               \
        RESULT_ENUM( prefix, PARTIAL_RESULTS,         3 ),   /**< Partial results */       \
        RESULT_ENUM( prefix, ERROR,                   4 ),   /**< Error */                 \
        RESULT_ENUM( prefix, BADARG,                  5 ),   /**< Bad Arguments */         \
        RESULT_ENUM( prefix, BADOPTION,               6 ),   /**< Mode not supported */    \
        RESULT_ENUM( prefix, UNSUPPORTED,             7 ),   /**< Unsupported function */  \
        RESULT_ENUM( prefix, INVALID_PACKET,          7008 ),   /**< Invalid packet */        \
        RESULT_ENUM( prefix, INVALID_SOCKET,          7009 ),   /**< Invalid socket */        \
        RESULT_ENUM( prefix, WAIT_ABORTED,            7010 ),   /**< Wait aborted */          \
        RESULT_ENUM( prefix, PORT_UNAVAILABLE,        7011 ),   /**< Port unavailable */      \
        RESULT_ENUM( prefix, IN_PROGRESS,             7012 ),   /**< Action in progress */    \
        RESULT_ENUM( prefix, IP_ADDRESS_IS_NOT_READY, 7013 ),   /**< IP_ADDRESS_IS_NOT_READY */

/******************************************************
 *                   Enumerations
 ******************************************************/

/** Enumeration of WICED interfaces. \n
 * @note The config interface is a virtual interface that shares the softAP interface
 */
typedef enum
{
    WICED_STA_INTERFACE      = WWD_STA_INTERFACE,             /**< STA or Client Interface  */
    WICED_AP_INTERFACE       = WWD_AP_INTERFACE,              /**< softAP Interface         */
    WICED_P2P_INTERFACE      = WWD_P2P_INTERFACE,             /**< P2P Interface            */
    WICED_ETHERNET_INTERFACE = WWD_ETHERNET_INTERFACE,        /**< Ethernet Interface       */


    WICED_INTERFACE_MAX,       /** DO NOT USE - MUST BE AFTER ALL NORMAL INTERFACES - used for counting interfaces */
    WICED_CONFIG_INTERFACE   = WICED_AP_INTERFACE | (1 << 7), /**< config softAP Interface  */
} wiced_interface_t;

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
