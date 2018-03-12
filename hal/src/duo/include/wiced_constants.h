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
        RESULT_ENUM( prefix, IP_ADDRESS_IS_NOT_READY, 7013 ),   /**< IP_ADDRESS_IS_NOT_READY */ \
        RESULT_ENUM( prefix, SOCKET_CLOSED,           7014 ),   /**< Socket closed */

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
