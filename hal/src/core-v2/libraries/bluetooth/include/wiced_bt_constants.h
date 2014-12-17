/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *  Defines common constants and types for the WICED Bluetooth Framework
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                     Macros
 ******************************************************/

#ifndef RESULT_ENUM
#define RESULT_ENUM( prefix, name, value )  prefix ## name = (value)
#endif /* ifndef RESULT_ENUM */

#define BT_RESULT_LIST( prefix ) \
        RESULT_ENUM( prefix, SUCCESS,                      0 ),   /**< Success */                                               \
        RESULT_ENUM( prefix, PENDING,                      1 ),   /**< Pending */                                               \
        RESULT_ENUM( prefix, TIMEOUT,                      2 ),   /**< Timeout */                                               \
        RESULT_ENUM( prefix, PARTIAL_RESULTS,              3 ),   /**< Partial results */                                       \
        RESULT_ENUM( prefix, ERROR,                        4 ),   /**< Error */                                                 \
        RESULT_ENUM( prefix, BADARG,                       5 ),   /**< Bad Arguments */                                         \
        RESULT_ENUM( prefix, BADOPTION,                    6 ),   /**< Mode not supported */                                    \
        RESULT_ENUM( prefix, UNSUPPORTED,                  7 ),   /**< Unsupported function */                                  \
        RESULT_ENUM( prefix, OUT_OF_HEAP_SPACE,            8 ),   /**< Dynamic memory space exhausted */                        \
        RESULT_ENUM( prefix, UNKNOWN_EVENT,             8029 ),   /**< Unknown event is received */                             \
        RESULT_ENUM( prefix, LIST_EMPTY,                8010 ),   /**< List is empty */                                         \
        RESULT_ENUM( prefix, ITEM_NOT_IN_LIST,          8011 ),   /**< Item not found in the list */                            \
        RESULT_ENUM( prefix, PACKET_DATA_OVERFLOW,      8012 ),   /**< Data overflow beyond the packet end */                   \
        RESULT_ENUM( prefix, PACKET_POOL_EXHAUSTED,     8013 ),   /**< All packets in the pool is in use */                     \
        RESULT_ENUM( prefix, PACKET_POOL_FATAL_ERROR,   8014 ),   /**< Packet pool fatal error such as permanent packet leak */ \
        RESULT_ENUM( prefix, UNKNOWN_PACKET,            8015 ),   /**< Unknown packet */                                        \
        RESULT_ENUM( prefix, PACKET_WRONG_OWNER,        8016 ),   /**< Packet is owned by another entity */                     \
        RESULT_ENUM( prefix, BUS_UNINITIALISED,         8017 ),   /**< Bluetooth bus isn't initialised */                       \
        RESULT_ENUM( prefix, MPAF_UNINITIALISED,        8018 ),   /**< MPAF framework isn't initialised */                      \
        RESULT_ENUM( prefix, RFCOMM_UNINITIALISED,      8019 ),   /**< RFCOMM protocol isn't initialised */                     \
        RESULT_ENUM( prefix, STACK_UNINITIALISED,       8020 ),   /**< SmartBridge isn't initialised */                         \
        RESULT_ENUM( prefix, SMARTBRIDGE_UNINITIALISED, 8021 ),   /**< Bluetooth stack isn't initialised */                     \
        RESULT_ENUM( prefix, ATT_CACHE_UNINITIALISED,   8022 ),   /**< Attribute cache isn't initialised */                     \
        RESULT_ENUM( prefix, MAX_CONNECTIONS_REACHED,   8023 ),   /**< Maximum number of connections is reached */              \
        RESULT_ENUM( prefix, SOCKET_IN_USE,             8024 ),   /**< Socket specified is in use */                            \
        RESULT_ENUM( prefix, SOCKET_NOT_CONNECTED,      8025 ),   /**< Socket is not connected or connection failed */          \
        RESULT_ENUM( prefix, ENCRYPTION_FAILED,         8026 ),   /**< Encryption failed */                                     \
        RESULT_ENUM( prefix, SCAN_IN_PROGRESS,          8027 ),   /**< Scan is in progress */                                   \
        RESULT_ENUM( prefix, CONNECT_IN_PROGRESS,       8028 ),   /**< Connect is in progress */                                \
        RESULT_ENUM( prefix, DISCONNECT_IN_PROGRESS,    8029 ),   /**< Disconnect is in progress */                             \
        RESULT_ENUM( prefix, DISCOVER_IN_PROGRESS,      8030 ),   /**< Discovery is in progress */                              \
        RESULT_ENUM( prefix, GATT_TIMEOUT,              8031 ),   /**< GATT timeout occured*/                                   \
        RESULT_ENUM( prefix, ATTRIBUTE_VALUE_TOO_LONG,  8032 ),   /**< Attribute value too long */                              \

/******************************************************
 *                    Constants
 ******************************************************/

/** @cond !ADDTHIS*/
#define WICED_BT_ADDRESS_BYTE_SIZE 6
/** @endcond */

/******************************************************
 *                   Enumerations
 ******************************************************/

/**
 * UUID size
 */
typedef enum
{
    UUID_16BIT  = 0x02, /**< 16-bit  */
    UUID_32BIT  = 0x04, /**< 32-bit  */
    UUID_128BIT = 0x10  /**< 128-bit */
} wiced_bt_uuid_size_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/**
 * Bluetooth Device Address (BD_ADDR)
 * A 48-bit address that uniquely identifies a Bluetooth device
 */
typedef struct
{
    uint8_t address[WICED_BT_ADDRESS_BYTE_SIZE]; /**< Address. 48-bit Bluetooth device address in a little-endian format */
} wiced_bt_device_address_t;


/**
 * Universally Unique Identifier (UUID)
 * A standardised format of string ID that uniquely identifies a Bluetooth service
 */
typedef struct
{
    union
    {
        uint16_t value_16_bit;                  /**< 16-bit UUID value.     */
        uint16_t value_128_bit[UUID_128BIT/2];  /**< 128-bit UUID value.    */
    } value;                                    /**< A union of UUID values */

    wiced_bt_uuid_size_t size;                  /**< UUID size              */

} wiced_bt_uuid_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif
