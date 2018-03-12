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
 * Defines STM32F2xx common peripheral structures, macros, constants and declares STM32F2xx peripheral API
 */
#pragma once
#include "platform_cmsis.h"
#include "platform_constants.h"
#include "stm32f2xx_adc.h"
#include "stm32f2xx_dma.h"
#include "stm32f2xx_exti.h"
#include "stm32f2xx_flash.h"
#include "stm32f2xx_gpio.h"
#include "stm32f2xx_i2c.h"
#include "stm32f2xx_iwdg.h"
#include "stm32f2xx_pwr.h"
#include "stm32f2xx_rtc.h"
#include "stm32f2xx_syscfg.h"
#include "stm32f2xx_tim.h"
#include "stm32f2xx_usart.h"
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
#define NUMBER_OF_GPIO_PORTS      (8)

/* Interrupt line 0 to 15. Each line is shared among the same numbered pins across all GPIO ports */
#define NUMBER_OF_GPIO_IRQ_LINES  (16)

/* USART1 to 6 */
#define NUMBER_OF_UART_PORTS      (6)

/* Invalid UART port number */
#define INVALID_UART_PORT_NUMBER  (0xff)

/* Default STDIO buffer size */
#ifndef STDIO_BUFFER_SIZE
#define STDIO_BUFFER_SIZE         (64)
#endif

/* SPI1 to SPI3 */
#define NUMBER_OF_SPI_PORTS       (3)

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/* GPIO port */
typedef GPIO_TypeDef  platform_gpio_port_t;

/* UART port */
typedef USART_TypeDef platform_uart_port_t;

/* SPI port */
typedef SPI_TypeDef   platform_spi_port_t;

/* I2C port */
typedef I2C_TypeDef   platform_i2c_port_t;

/* GPIO alternate function */
typedef uint8_t       platform_gpio_alternate_function_t;

/* Peripheral clock function */
typedef void (*platform_peripheral_clock_function_t)(uint32_t clock, FunctionalState state );

typedef DMA_TypeDef     dma_registers_t;
typedef FunctionalState functional_state_t;
typedef uint32_t        peripheral_clock_t;

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    DMA_TypeDef*        controller;
    DMA_Stream_TypeDef* stream;
    uint32_t            channel;
    IRQn_Type           irq_vector;
    uint32_t            complete_flags;
    uint32_t            error_flags;
} platform_dma_config_t;

typedef struct
{
    platform_gpio_port_t* port;       /* GPIO port. platform_gpio_port_t is defined in <WICED-SDK>/MCU/<MCU>/platform_mcu_interface.h */
    uint8_t               pin_number; /* pin number. Valid range is defined in <WICED-SDK>/MCU/<MCU>/platform_mcu_interface.h         */
} platform_gpio_t;

typedef struct
{
    ADC_TypeDef*           port;
    uint8_t                channel;
    uint32_t               adc_peripheral_clock;
    uint8_t                rank;
    const platform_gpio_t* pin;
} platform_adc_t;

typedef struct
{
    TIM_TypeDef*           tim;
    uint8_t                channel;
    uint32_t               tim_peripheral_clock;
    uint8_t                gpio_af;
    const platform_gpio_t* pin;
} platform_pwm_t;

/* DMA can be enabled by setting SPI_USE_DMA */
typedef struct
{
    platform_spi_port_t*                 port;
    uint8_t                              gpio_af;
    uint32_t                             peripheral_clock_reg;
    platform_peripheral_clock_function_t peripheral_clock_func;
    const platform_gpio_t*               pin_mosi;
    const platform_gpio_t*               pin_miso;
    const platform_gpio_t*               pin_clock;
    platform_dma_config_t                tx_dma;
    platform_dma_config_t                rx_dma;
} platform_spi_t;

typedef struct
{
    uint8_t unimplemented;
} platform_spi_slave_driver_t;

typedef struct
{
    platform_i2c_port_t*   port;
    const platform_gpio_t* pin_scl;
    const platform_gpio_t* pin_sda;
    uint32_t               peripheral_clock_reg;
    dma_registers_t*       tx_dma;
    peripheral_clock_t     tx_dma_peripheral_clock;
    DMA_Stream_TypeDef*    tx_dma_stream;
    DMA_Stream_TypeDef*    rx_dma_stream;
    int                    tx_dma_stream_id;
    int                    rx_dma_stream_id;
    uint32_t               tx_dma_channel;
    uint32_t               rx_dma_channel;
    uint8_t                gpio_af;
} platform_i2c_t;

typedef struct
{
    platform_uart_port_t*  port;
    const platform_gpio_t* tx_pin;
    const platform_gpio_t* rx_pin;
    const platform_gpio_t* cts_pin;
    const platform_gpio_t* rts_pin;
    platform_dma_config_t  tx_dma_config;
    platform_dma_config_t  rx_dma_config;
} platform_uart_t;

typedef struct
{
    platform_uart_t*           peripheral;
    wiced_ring_buffer_t*       rx_buffer;
    host_semaphore_type_t      rx_complete;
    host_semaphore_type_t      tx_complete;
    volatile uint32_t          tx_size;
    volatile uint32_t          rx_size;
    volatile platform_result_t last_receive_result;
    volatile platform_result_t last_transmit_result;
} platform_uart_driver_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

platform_result_t platform_gpio_irq_manager_init      ( void );
uint8_t           platform_gpio_get_port_number       ( platform_gpio_port_t* gpio_port );
platform_result_t platform_gpio_set_alternate_function( platform_gpio_port_t* gpio_port, uint8_t pin_number, GPIOOType_TypeDef output_type, GPIOPuPd_TypeDef pull_up_down_type, uint8_t alternation_function );

platform_result_t platform_watchdog_init              ( void );
platform_result_t platform_mcu_powersave_init         ( void );

platform_result_t platform_rtc_init                   ( void );
platform_result_t platform_rtc_enter_powersave        ( void );
platform_result_t platform_rtc_abort_powersave        ( void );
platform_result_t platform_rtc_exit_powersave         ( uint32_t requested_sleep_time, uint32_t *cpu_sleep_time );

uint8_t           platform_uart_get_port_number       ( platform_uart_port_t* uart );
void              platform_uart_irq                   ( platform_uart_driver_t* driver );
void              platform_uart_tx_dma_irq            ( platform_uart_driver_t* driver );
void              platform_uart_rx_dma_irq            ( platform_uart_driver_t* driver );

uint8_t           platform_spi_get_port_number        ( platform_spi_port_t* spi );

platform_result_t platform_filesystem_init            ( void );
#ifdef __cplusplus
} /* extern "C" */
#endif

