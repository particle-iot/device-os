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

#include "platform_peripheral.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/
#ifndef UART_RX_FIFO_SIZE
#define UART_RX_FIFO_SIZE (3000)
#endif
/******************************************************
 *                    Constants
 ******************************************************/
/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    WICED_BT_PIN_POWER,
    WICED_BT_PIN_RESET,
    WICED_BT_PIN_HOST_WAKE,
    WICED_BT_PIN_DEVICE_WAKE,
    WICED_BT_PIN_MAX,
} wiced_bt_control_pin_t;

typedef enum
{
    WICED_BT_PIN_UART_TX,
    WICED_BT_PIN_UART_RX,
    WICED_BT_PIN_UART_CTS,
    WICED_BT_PIN_UART_RTS,
} wiced_bt_uart_pin_t;

typedef enum
{
    PATCHRAM_DOWNLOAD_MODE_NO_MINIDRV_CMD,
    PATCHRAM_DOWNLOAD_MODE_MINIDRV_CMD,
} wiced_bt_patchram_download_mode_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct
{
    uint32_t                          patchram_download_baud_rate;
    wiced_bt_patchram_download_mode_t patchram_download_mode;
    uint32_t                          featured_baud_rate;
} platform_bluetooth_config_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

//TODO: put all these variables into a single structure.
/* Variables to be defined by the Bluetooth supporting platform */
extern const platform_gpio_t*        wiced_bt_control_pins[];
extern const platform_gpio_t*        wiced_bt_uart_pins[];
extern const platform_uart_t*        wiced_bt_uart_peripheral;
extern       platform_uart_driver_t* wiced_bt_uart_driver;
extern const platform_uart_config_t  wiced_bt_uart_config;
extern const platform_bluetooth_config_t wiced_bt_config;

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif
