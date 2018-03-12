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

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    WICED_NFC_I2C_SDA,
    WICED_NFC_I2C_SCL,
} wiced_nfc_i2c_pin_t;

typedef enum
{
    WICED_NFC_PIN_POWER,
    WICED_NFC_PIN_WAKE,
    WICED_NFC_PIN_TRANS_SELECT,
    WICED_NFC_PIN_IRQ_REQ,
} wiced_nfc_control_pin_t;

typedef enum
{
    WICED_NFC_PIN_UART_TX,
    WICED_NFC_PIN_UART_RX,
    WICED_NFC_PIN_UART_CTS,
    WICED_NFC_PIN_UART_RTS,
} wiced_nfc_uart_pin_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/* Variables to be defined by the NFC supporting platform */
extern const platform_gpio_t*        wiced_nfc_control_pins[];
extern const platform_gpio_t*        wiced_nfc_uart_pins[];
extern const platform_uart_t*        wiced_nfc_uart_peripheral;
extern       platform_uart_driver_t* wiced_nfc_uart_driver;
extern const platform_uart_config_t  wiced_nfc_uart_config;

/******************************************************
 *               Function Declarations
 ******************************************************/


#ifdef __cplusplus
} /* extern "C" */
#endif
