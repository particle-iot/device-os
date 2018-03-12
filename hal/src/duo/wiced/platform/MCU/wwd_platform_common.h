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

/******************************************************
 *                 Type Definitions
 ******************************************************/

/**
 * WLAN control pins
 */
typedef enum
{
    WWD_PIN_POWER,
    WWD_PIN_RESET,
    WWD_PIN_32K_CLK,
    WWD_PIN_BOOTSTRAP_0,
    WWD_PIN_BOOTSTRAP_1,
    WWD_PIN_CONTROL_MAX,
} wwd_control_pin_t;

/**
 * WLAN SDIO pins
 */
typedef enum
{
    WWD_PIN_SDIO_OOB_IRQ,
    WWD_PIN_SDIO_CLK,
    WWD_PIN_SDIO_CMD,
    WWD_PIN_SDIO_D0,
    WWD_PIN_SDIO_D1,
    WWD_PIN_SDIO_D2,
    WWD_PIN_SDIO_D3,
    WWD_PIN_SDIO_MAX,
} wwd_sdio_pin_t;

/**
 * WLAN SPI pins
 */
typedef enum
{
    WWD_PIN_SPI_IRQ,
    WWD_PIN_SPI_CS,
    WWD_PIN_SPI_CLK,
    WWD_PIN_SPI_MOSI,
    WWD_PIN_SPI_MISO,
    WWD_PIN_SPI_MAX,
} wwd_spi_pin_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/* Externed from <WICED-SDK>/platforms/<Platform>/platform.c */
extern const platform_gpio_t wifi_control_pins[];
extern const platform_gpio_t wifi_sdio_pins   [];
extern const platform_gpio_t wifi_spi_pins    [];

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /*extern "C" */
#endif
