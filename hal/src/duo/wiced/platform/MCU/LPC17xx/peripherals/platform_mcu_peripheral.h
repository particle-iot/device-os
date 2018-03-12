/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 * Defines LPC17xx common peripheral structures, macros, constants and declares  peripheral API
 */
#pragma once

#include "platform_cmsis.h"
#include "chip.h"
#include "platform_constants.h"
#include "platform_peripheral.h"
#include "wwd_constants.h"
#include "RTOS/wwd_rtos_interface.h"
#include "ring_buffer.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/* GPIOA to I */
#define NUMBER_OF_GPIO_PORTS     (8)

/* USART0,2,3 */
#define NUMBER_OF_UART_PORTS     (4)

#define NUMBER_OF_SPI_PORTS      (1)
/* Invalid UART port number */
#define INVALID_UART_PORT_NUMBER (0xff)

#define NUMBER_OF_P0_GPIO_IRQ    (29)

#define NUMBER_OF_P2_GPIO_IRQ    (14)


/* Default STDIO buffer size */
#ifndef STDIO_BUFFER_SIZE
#define STDIO_BUFFER_SIZE        (64)
#endif

#define LPC_PIN_INPUT            ((uint8_t)0)
#define LPC_PIN_OUTPUT           ((uint8_t)1)

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef LPC_USART_T          platform_uart_port_t;
typedef LPC_GPIO_T           platform_gpio_port_t;
typedef uint8_t              platform_gpio_alternate_function_t;
typedef LPC_SPI_T            platform_spi_port_t;
typedef IP_GPIOPININT_MODE_T platform_gpio_interrupt_trigger;

typedef void (*platform_gpio_irq_handler_t)( void* arg );

/* Peripheral clock function */
//typedef void (*platform_peripheral_clock_function_t)(uint32_t clock, FunctionalState state );

/******************************************************
 *                    Structures
 ******************************************************/
typedef struct
{
    uint8_t port; /* GPIO port. platform_gpio_port_t is defined in <WICED-SDK>/MCU/<MCU>/platform_mcu_interface.h */
    uint8_t pin;  /* pin number. Valid range is defined in <WICED-SDK>/MCU/<MCU>/platform_mcu_interface.h         */
} platform_gpio_t;

typedef struct
{
    platform_gpio_t config;
    uint16_t        mode;   /*please use SCU function and mode selection definitions from scu_17xx_40xx.h*/
    uint8_t         function;
}platform_gpio_config_t;

typedef struct
{
    platform_uart_port_t*  uart_base;
    platform_gpio_config_t tx_pin;
    platform_gpio_config_t rx_pin;
    platform_gpio_config_t cts_pin;
    platform_gpio_config_t rts_pin;
}platform_uart_t;

typedef struct
{
    platform_uart_t*           interface;
    wiced_ring_buffer_t*       rx_buffer;
    host_semaphore_type_t      rx_complete;
    host_semaphore_type_t      tx_complete;
    volatile uint32_t          tx_size;
    volatile uint32_t          rx_size;
    volatile platform_result_t last_receive_result;
    volatile platform_result_t last_transmit_result;
} platform_uart_driver_t;

typedef struct
{
    platform_spi_port_t*   spi_base;
    platform_gpio_config_t clock;
    platform_gpio_config_t mosi;
    platform_gpio_config_t miso;
    platform_gpio_t        cs;
    platform_gpio_t        irq;
}platform_spi_t;

typedef struct
{
    uint8_t unimplemented;
} platform_spi_slave_driver_t;

typedef struct
{
    CHIP_SPI_MODE_T          master_slave_mode;
    CHIP_SPI_CONFIG_FORMAT_T data_clock_format;
    uint32_t                 bit_rate;
}platform_spi_peripheral_config_t;

typedef struct
{
    uint8_t unimplemented;
} platform_adc_t;

typedef struct
{
    uint8_t unimplemented;
} platform_pwm_t;

typedef struct
{
    uint8_t unimplemented;
} platform_i2c_t;

typedef void (*platform_peripheral_clock_function_t)(CHIP_SYSCTL_CLOCK_T clock);

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

platform_result_t platform_gpio_set_alternate_function( const platform_gpio_config_t* port_config );
uint8_t           platform_gpio_get_port_number       ( platform_gpio_port_t* gpio_port );
uint8_t           platform_uart_get_port_number       ( platform_uart_port_t* uart );
platform_result_t platform_rtc_init                   ( void );
uint8_t           platform_spi_get_port_number        ( platform_spi_port_t* spi );
void              platform_uart_irq                   ( platform_uart_driver_t* driver );

#ifdef __cplusplus
} /* extern "C" */
#endif
