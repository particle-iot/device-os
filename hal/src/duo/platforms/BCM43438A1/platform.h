/*
 * Copyright 2016, RedBear
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of RedBear;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of RedBear Corporation.
 */

/** @file
 * Defines peripherals available for use on RedBear Duo board
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
/*
Notes
1. These mappings are defined in <WICED-SDK>/platforms/RBL205RGAP6212/platform.c
2. STM32F2xx Datasheet  -> http://www.st.com/web/en/resource/technical/document/datasheet/CD00237391.pdf
3. STM32F2xx Ref Manual -> http://www.st.com/web/en/resource/technical/document/reference_manual/CD00225773.pdf
*/

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    WICED_GPIO_1,
    WICED_GPIO_2,
    WICED_GPIO_3,
    WICED_GPIO_4,
    WICED_GPIO_5,
    WICED_GPIO_6,
    WICED_GPIO_7,
    WICED_GPIO_8,
    WICED_GPIO_9,
    WICED_GPIO_10,
    WICED_GPIO_11,
    WICED_GPIO_12,
    WICED_GPIO_13,
    WICED_GPIO_14,
    WICED_GPIO_15,
    WICED_GPIO_16,
    WICED_GPIO_17,
    WICED_GPIO_18,

    // LED & Button
    WICED_GPIO_19,
    WICED_GPIO_20,
    WICED_GPIO_21,
    WICED_GPIO_22,

    // SPI Flash
    WICED_GPIO_23, // FLASH_SPI_CS
    WICED_GPIO_24, // FLASH_SPI_CLK
    WICED_GPIO_25, // FLASH_SPI_MOSI
    WICED_GPIO_26, // FLASH_SPI_MISO

    WICED_GPIO_MAX, /* Denotes the total number of GPIO port aliases. Not a valid GPIO alias */
    WICED_GPIO_32BIT = 0x7FFFFFFF,
} wiced_gpio_t;

typedef enum
{
    WICED_ADC_1,
    WICED_ADC_2,
    WICED_ADC_3,
    WICED_ADC_4,
    WICED_ADC_5,
    WICED_ADC_6,
    WICED_ADC_7,
    WICED_ADC_8,

    WICED_ADC_MAX, /* Denotes the total number of ADC port aliases. Not a valid ADC alias */
    WICED_ADC_32BIT = 0x7FFFFFFF,
} wiced_adc_t;

typedef enum
{
    WICED_PWM_1,
    WICED_PWM_2,
    WICED_PWM_3,
    WICED_PWM_4,
    WICED_PWM_5,
    WICED_PWM_6,
    WICED_PWM_7,
    WICED_PWM_8,
    WICED_PWM_9,
    WICED_PWM_10,
    WICED_PWM_11,
    WICED_PWM_12,
    WICED_PWM_13,

    WICED_PWM_14,
    WICED_PWM_15,
    WICED_PWM_16,

    WICED_PWM_MAX, /* Denotes the total number of PWM port aliases. Not a valid PWM alias */
    WICED_PWM_32BIT = 0x7FFFFFFF,
} wiced_pwm_t;

typedef enum
{
    WICED_I2C_1,
    WICED_I2C_MAX,
    WICED_I2C_32BIT = 0x7FFFFFFF,
} wiced_i2c_t;

typedef enum
{
    WICED_SPI_1,
    WICED_SPI_2,
    WICED_SPI_3,
    WICED_SPI_MAX, /* Denotes the total number of SPI port aliases. Not a valid SPI alias */
    WICED_SPI_32BIT = 0x7FFFFFFF,
} wiced_spi_t;

typedef enum
{
    WICED_I2S_1,
    WICED_I2S_MAX, /* Denotes the total number of I2S port aliases.  Not a valid I2S alias */
    WICED_I2S_32BIT = 0x7FFFFFFF
} wiced_i2s_t;

typedef enum
{
    WICED_UART_1,
    WICED_UART_2,
    WICED_UART_MAX, /* Denotes the total number of UART port aliases. Not a valid UART alias */
    WICED_UART_32BIT = 0x7FFFFFFF,
} wiced_uart_t;

/* Logical Button-ids which map to phyiscal buttons on the board */
typedef enum
{
    PLATFORM_BUTTON_1,
    PLATFORM_BUTTON_MAX, /* Denotes the total number of Buttons on the board. Not a valid Button Alias */
} platform_button_t;

/******************************************************
 *                    Constants
 ******************************************************/

#define WICED_PLATFORM_BUTTON_COUNT        ( 1 )

/* UART port used for standard I/O */
#define STDIO_UART                         ( WICED_UART_1 )

/* SPI flash is present */
#define WICED_PLATFORM_INCLUDES_SPI_FLASH
#define WICED_SPI_FLASH_CS                 ( WICED_GPIO_23 )

#define DAC_EXTERNAL_OSCILLATOR

/* Components connected to external I/Os */
#define WICED_LED1                         ( WICED_GPIO_19 )
#define WICED_LED2                         ( WICED_GPIO_20 )
#define WICED_LED3                         ( WICED_GPIO_21 )
#define WICED_LED4                         ( WICED_GPIO_8 )
#define WICED_BUTTON1                      ( WICED_GPIO_22 )
#define WICED_BUTTON2                      ( WICED_GPIO_MAX )
#define WICED_THERMISTOR                   ( WICED_GPIO_MAX )

/* I/O connection <-> Peripheral Connections */
#define WICED_LED1_JOINS_PWM               ( WICED_PWM_14 )
#define WICED_LED2_JOINS_PWM               ( WICED_PWM_15 )
#define WICED_LED3_JOINS_PWM               ( WICED_PWM_16 )
//#define WICED_THERMISTOR_JOINS_ADC       ( WICED_ADC_3 )

/*  Bootloader OTA/OTA2 LED to flash while "Factory Reset" button held */
#define PLATFORM_FACTORY_RESET_LED_GPIO     ( WICED_LED3 )
#define PLATFORM_FACTORY_RESET_LED_ON_STATE ( WICED_ACTIVE_LOW )

/* Bootloader OTA and OTA2 factory reset during settings */
#define PLATFORM_FACTORY_RESET_BUTTON_GPIO   ( WICED_BUTTON1 )
#define PLATFORM_FACTORY_RESET_PRESSED_STATE (   0  )
#define PLATFORM_FACTORY_RESET_CHECK_PERIOD  (  100 )


#ifdef __cplusplus
} /*extern "C" */
#endif
