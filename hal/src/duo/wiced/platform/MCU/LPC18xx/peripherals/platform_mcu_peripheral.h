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

#include "platform_constants.h"
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

#define NUMBER_OF_GPIO_IRQ_LINES (8)

#define NUMBER_OF_SPI_PORTS      (1)
/* Invalid UART port number */
#define INVALID_UART_PORT_NUMBER (0xff)

/* Default STDIO buffer size */
#ifndef STDIO_BUFFER_SIZE
#define STDIO_BUFFER_SIZE        (64)
#endif

#define LPC_PIN_INPUT            ((uint8_t)0)
#define LPC_PIN_OUTPUT           ((uint8_t)1)

/******************************************************
 *                   Enumerations
 ******************************************************/
typedef enum
{
    LPC_GPIO_PORT_0    = 0,
    LPC_GPIO_PORT_1    = 1,
    LPC_GPIO_PORT_2    = 2,
    LPC_GPIO_PORT_3    = 3,
    LPC_GPIO_PORT_4    = 4,
    LPC_GPIO_PORT_5    = 5,
    LPC_GPIO_PORT_6    = 6,
    LPC_GPIO_PORT_7    = 7
} lpc_gpio_port_t;

typedef enum
{
    LPC_PIN_GROUP_0      = 0,
    LPC_PIN_GROUP_1      = 1,
    LPC_PIN_GROUP_2      = 2,
    LPC_PIN_GROUP_3      = 3,
    LPC_PIN_GROUP_4      = 4,
    LPC_PIN_GROUP_5      = 5,
    LPC_PIN_GROUP_6      = 6,
    LPC_PIN_GROUP_7      = 7,
    LPC_PIN_GROUP_8      = 8,
    LPC_PIN_GROUP_9      = 9,
    LPC_PIN_GROUP_A      = 10,
    LPC_PIN_GROUP_B      = 11,
    LPC_PIN_GROUP_C      = 12,
    LPC_PIN_GROUP_D      = 13,
    LPC_PIN_GROUP_E      = 14,
    LPC_PIN_GROUP_F      = 15,
    LPC_PIN_GROUP_CLK    = 24,
} lpc_pin_group_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/
/* LPC USART structure.
 * For more details see uart_18xx_43xx.h
 */
typedef void* platform_uart_port_t;
typedef void* platform_spi_port_t;
typedef void* platform_spifi_port_t;
typedef void* platform_sdio_port_t;

typedef struct
{
    uint8_t to_be_filled;
} platform_gpio_port_t;

typedef uint8_t platform_gpio_alternate_function_t;

/* Changed temporary to get stuff working */
typedef struct
{
        uint8_t to_be_filled;
} platform_gpio_interrupt_trigger;

typedef void (*platform_gpio_irq_handler_t)( void* arg );

/******************************************************
 *                    Structures
 ******************************************************/
/* This generic structure specifies a hardware pin. */
typedef struct
{
    lpc_pin_group_t group;           /* GROUP of pins to which pin is connected. */
    uint8_t         pin;            /* Pin within the port to which pin is connected.   */
    uint16_t        mode_function;  /* SCU function and mode selection definitions from scu_18xx_43xx.h*/
} platform_pin_t;

/* GPIO */
/*~~~~~~*/
typedef struct
{
    platform_pin_t  hw_pin;          /* The actual hardwar pin the GPIO is connected to. */
    lpc_gpio_port_t gpio_port;       /* GPIO port. platform_gpio_port_t is defined in <WICED-SDK>/MCU/<MCU>/platform_mcu_interface.h */
    uint8_t         gpio_pin;        /* pin number. Valid range is defined in <WICED-SDK>/MCU/<MCU>/platform_mcu_interface.h         */
} platform_gpio_t;


/* USART */
/*~~~~~~~*/
typedef struct
{
    platform_uart_port_t*   uart_base;
    platform_pin_t          tx_pin;
    platform_pin_t          rx_pin;
    platform_pin_t          cts_pin;
    platform_pin_t          rts_pin;
} platform_uart_t;

typedef struct
{
    platform_uart_t*        interface;
    wiced_ring_buffer_t*    rx_buffer;
    host_semaphore_type_t   rx_complete;
    host_semaphore_type_t   tx_complete;
    volatile uint32_t       tx_size;
    volatile uint32_t       rx_size;
    volatile platform_result_t last_receive_result;
    volatile platform_result_t last_transmit_result;
} platform_uart_driver_t;

/* SPI */
/*~~~~~*/
typedef struct
{
    uint8_t                 enable_cs_emulation;    /* CS will be emulated through software. Should be used for WLAN SPI interface. */
    platform_spi_port_t*    spi_base;
    platform_pin_t          clock;
    platform_pin_t          mosi;
    platform_pin_t          miso;
    host_semaphore_type_t   in_use_semaphore;
    wiced_bool_t            semaphore_is_inited;
} platform_spi_t;

typedef struct
{
    uint8_t unimplemented;
} platform_spi_slave_driver_t;

typedef struct
{
    uint32_t bit_rate;
} platform_spi_peripheral_config_t;

/* SPIFI */
/*~~~~~~~*/
typedef struct
{
    platform_spifi_port_t*  spifi_base;
    platform_pin_t          clock;
    platform_pin_t          miso;
    platform_pin_t          mosi;
    platform_pin_t          sio2;
    platform_pin_t          sio3;
    platform_pin_t          cs;
    host_semaphore_type_t   in_use_semaphore;
    wiced_bool_t            semaphore_is_inited;
} platform_spifi_t;

/* ADC */
/*~~~~~*/
typedef struct
{
    uint8_t unimplemented;
} platform_adc_t;

/* PWM */
/*~~~~~*/
typedef struct
{
    uint8_t unimplemented;
} platform_pwm_t;

/* I2c */
/*~~~~~*/
typedef struct
{
    uint8_t unimplemented;
} platform_i2c_t;

/* Changed temporary to get stuff working */
typedef void (*platform_peripheral_clock_function_t)( int clock );

/* SDIO */
/*~~~~~~~*/
typedef struct
{
    platform_sdio_port_t*   base;
    platform_pin_t          clock;
    platform_pin_t          D0;
    platform_pin_t          D1;
    platform_pin_t          D2;
    platform_pin_t          D3;
    platform_pin_t          cmd;
    host_semaphore_type_t   in_use_semaphore;
    wiced_bool_t            semaphore_is_inited;
} platform_sdio_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
void              platform_uart_irq     ( platform_uart_driver_t* driver );
platform_result_t platform_gpio_irq_init( void );
platform_result_t platform_pin_set_alternate_function( const platform_pin_t* pin );

#ifdef __cplusplus
} /* extern "C" */
#endif
