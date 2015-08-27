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
