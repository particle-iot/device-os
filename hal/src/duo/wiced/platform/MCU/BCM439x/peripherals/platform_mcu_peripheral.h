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

#include "platform_constants.h"
#include "wwd_constants.h"
#include "ring_buffer.h"
#include "wwd_rtos.h"
//#include "RTOS/wwd_rtos_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/* Default STDIO buffer size */
#ifndef STDIO_BUFFER_SIZE
#define STDIO_BUFFER_SIZE  ( 64 )
#endif

/******************************************************
 *                   Enumerations
 ******************************************************/

/* 4390 pins */
typedef enum
{
    PA00,  /* UART1_RTS_N_A0 */
    PA01,  /* UART1_CTS_N_A1 */
    PA02,  /* JTAG_TMS_A2 */
    PA03,  /* JTAG_TCK_A3 */
    PA04,  /* JTAG_TDI_A4 */
    PA05,  /* JTAG_TDO_A5 */
    PA06,  /* I2S_DI_A6 */
    PA07,  /* I2S_WS_A7 */
    PA08,  /* I2S_D0_A8 */
    PA09,  /* I2S_CLK_A9 */
    PA10,  /* WAKE_A10 */
    PA11,  /* GPIO_A11 */
    PA12,  /* UART1_RX_GPIO_A5 */
    PA13,  /* UART1_TX_GPIO_A4 */
    PB00,
    PB01,
    PB02,
    PB03,
    PB04,
    PB05,
    PB06,
    PB07,
    PB08,
    PB09,
    PB10,
    PB11,
    PB12,  /* UART4_TX_RF_SW_CTRL_8 */
    PB13,  /* UART4_RX_RF_SW_CTRL_8 */
    DEBUG_EN,
    SFLASH_SPI_CLK,  /* Usable only if serial flash pins are exposed on the SiP */
    SFLASH_SPI_MOSI, /* Usable only if serial flash pins are exposed on the SiP */
    SFLASH_SPI_MISO, /* Usable only if serial flash pins are exposed on the SiP */
    SFLASH_SPI_CS,   /* Usable only if serial flash pins are exposed on the SiP */
    PIN_MAX  /* Denotes maximum value. Not a valid pin */
} platform_pin_t;

/* Pin alternate functions */
typedef enum
{
    PIN_FUNCTION_GPIO,
    PIN_FUNCTION_UART1,
    PIN_FUNCTION_UART2,
    PIN_FUNCTION_UART3_TX,
    PIN_FUNCTION_UART3_RX,
    PIN_FUNCTION_SPI1,
    PIN_FUNCTION_SPI2,
    PIN_FUNCTION_I2C,
    PIN_FUNCTION_I2S_SLAVE,
    PIN_FUNCTION_I2S_MASTER,
    PIN_FUNCTION_JTAG,
    PIN_FUNCTION_MAX /* Denotes max value. Not a valid alternate function */
} platform_pin_function_t;

typedef enum
{
    UART1,
    UART2,
    UART3,
    UART4
} platform_uart_port_t;

typedef enum
{
    SPI1,
    SPI2,
    SPI_MAX
} platform_spi_port_t;

typedef enum
{
    SPI_SLAVE_STATE_RECEIVING_COMMAND,
    SPI_SLAVE_STATE_TRANSFERING_DATA
} platform_spi_slave_state_t;

typedef enum
{
    PTU1_UART1,
    PTU1_SPI1,
    PTU1_PERIPHERAL_MAX
} platform_ptu1_peripheral_t;

typedef enum
{
    PTU2_UART2,
    PTU2_SPI2,
    PTU2_PERIPHERAL_MAX
} platform_ptu2_peripheral_t;

typedef enum
{
    WAKEUP_PIN_ACTIVE_LOW,
    WAKEUP_PIN_ACTIVE_HIGH
} platform_wakeup_pin_polarity_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    uint32_t output_disable;
    uint32_t pullup_enable;
    uint32_t pulldown_enable;
    uint32_t schmitt_trigger_input_enable;
    uint32_t drive_strength; // Add one to get drive strength in milliamps (1mA - 8mA)
    uint32_t input_disable;
} platform_pin_gpio_config_t;

typedef struct
{
    platform_pin_t pin;
} platform_gpio_t;

typedef struct
{
    platform_uart_port_t   port;
    const platform_gpio_t* tx_pin;
    const platform_gpio_t* rx_pin;
    const platform_gpio_t* cts_pin;
    const platform_gpio_t* rts_pin;
} platform_uart_t;

typedef struct
{
    wiced_ring_buffer_t*   rx_buffer;
    host_semaphore_type_t  rx_complete;
    host_semaphore_type_t  tx_complete;
    volatile uint32_t      tx_size;
    volatile uint32_t      rx_size;
    platform_uart_t*       interface;
    wiced_bool_t           hw_flow_control_is_on;
    const platform_gpio_t* rts_gpio;
    wiced_bool_t           rts_is_deasserted;
} platform_uart_driver_t;

typedef struct
{
    platform_spi_port_t    port;
    wiced_bool_t           is_master_supported;
    wiced_bool_t           is_slave_supported;
    const platform_gpio_t* mosi_pin;
    const platform_gpio_t* miso_pin;
    const platform_gpio_t* clock_pin;
    const platform_gpio_t* slave_chip_select_pin;
    const platform_gpio_t* slave_ready_pin;
} platform_spi_t;

typedef struct
{
    const platform_spi_t*                   peripheral;
    const struct platform_spi_slave_config* config;
    host_semaphore_type_t                   transfer_complete;
    platform_spi_slave_state_t              state;
} platform_spi_slave_driver_t;

typedef struct
{
    const platform_gpio_t* scl_pin;
    const platform_gpio_t* sda_pin;
} platform_i2c_t;

typedef struct
{
    uint32_t dummy; /* Interface unimplemented */
} platform_pwm_t;

typedef struct
{
    uint32_t dummy; /* Interface unimplemented */
} platform_adc_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/* GPIO and Pin Management Function(s) */
platform_result_t platform_gpio_irq_manager_init    ( void );
platform_result_t platform_pin_init                 ( platform_pin_t pin, platform_pin_gpio_config_t config );
platform_result_t platform_pin_set_function         ( platform_pin_t pin, platform_pin_function_t function );
platform_result_t platform_wakeup_pin_enable        ( platform_pin_t wakeup_pin, platform_wakeup_pin_polarity_t polarity );
platform_result_t platform_wakeup_pin_disable       ( platform_pin_t wakeup_pin );
wiced_bool_t      platform_wakeup_pin_is_asserted   ( platform_pin_t wakeup_pin );

/* Peripheral Transport Unit (PTU) Functions */
platform_result_t platform_ptu1_init                ( void );
platform_result_t platform_ptu1_enable              ( platform_ptu1_peripheral_t peripheral );
platform_result_t platform_ptu1_disable             ( platform_ptu1_peripheral_t peripheral );
platform_result_t platform_ptu1_enable_interrupt    ( platform_ptu1_peripheral_t peripheral );
platform_result_t platform_ptu1_disable_interrupt   ( platform_ptu1_peripheral_t peripheral );
platform_result_t platform_ptu2_init                ( void );
platform_result_t platform_ptu2_enable              ( platform_ptu2_peripheral_t peripheral );
platform_result_t platform_ptu2_disable             ( platform_ptu2_peripheral_t peripheral );
platform_result_t platform_ptu2_enable_interrupt    ( platform_ptu2_peripheral_t peripheral );
platform_result_t platform_ptu2_disable_interrupt   ( platform_ptu2_peripheral_t peripheral );
platform_result_t platform_ptu2_enable_serial_flash ( void );
platform_result_t platform_ptu2_disable_serial_flash( void );

/* Power Management Unit (PMU) Functions */
platform_result_t platform_pmu_enter_pds_mode       ( uint32_t time_to_sleep_ms, uint32_t* time_slept_ms );
wiced_bool_t      platform_pmu_wifi_is_on           ( void );
void              platform_pmu_wifi_on              ( void );
void              platform_pmu_wifi_off             ( void );
void              platform_pmu_wifi_sram_init       ( void );
void              platform_pmu_wifi_sram_on         ( void );
void              platform_pmu_wifi_sram_off        ( void );
void              platform_pmu_wifi_needed          ( void );
void              platform_pmu_wifi_not_needed      ( void );
void              platform_pmu_wifi_wake_up         ( void );
void              platform_pmu_wifi_allowed_to_sleep( void );

/* Initialisation Function(s) */
platform_result_t platform_watchdog_init            ( void );
platform_result_t platform_reinit_wlan_stdio_uart   ( void );

platform_result_t platform_watchdog_force_reset     ( void );
void              platform_watchdog_disable         ( void );
void              platform_apps_core_init           ( void );
platform_result_t platform_filesystem_init          ( void );
void              platform_sflash_init              ( void );
platform_result_t platform_otp_setup                ( void );

/* Interrupt Handler Function(s) */
void              platform_gpio_irq                 ( void );
void              platform_pmu_wakeup_irq           ( void );
void              platform_uart_irq                 ( platform_uart_driver_t* driver );
void              platform_spi_slave_irq            ( platform_spi_slave_driver_t* driver );

#ifdef __cplusplus
} /*extern "C" */
#endif
