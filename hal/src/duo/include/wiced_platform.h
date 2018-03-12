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
 *  Defines functions that access platform specific peripherals
 *
 */

#pragma once

#include "wiced_result.h"
#include "wiced_utilities.h"
#include "wwd_constants.h"
#include "platform_peripheral.h"
#include "platform.h" /* This file is unique for each platform */
#include "platform_dct.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 * @cond                Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define WICED_I2C_START_FLAG                    (1U << 0)
#define WICED_I2C_REPEATED_START_FLAG           (1U << 1)
#define WICED_I2C_STOP_FLAG                     (1U << 2)

#define WICED_GPIO_NONE ((wiced_gpio_t)0x7fffffff)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    WICED_ACTIVE_LOW   = 0,
    WICED_ACTIVE_HIGH  = 1,
} wiced_active_state_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef platform_pin_config_t                   wiced_gpio_config_t;

typedef platform_gpio_irq_trigger_t             wiced_gpio_irq_trigger_t;

typedef platform_gpio_irq_callback_t            wiced_gpio_irq_handler_t;

typedef platform_uart_config_t                  wiced_uart_config_t;

typedef platform_i2c_bus_address_width_t        wiced_i2c_bus_address_width_t;

typedef platform_i2c_speed_mode_t               wiced_i2c_speed_mode_t;

typedef platform_i2c_message_t                  wiced_i2c_message_t;

typedef platform_spi_message_segment_t          wiced_spi_message_segment_t;

typedef platform_rtc_time_t                     wiced_rtc_time_t;

typedef platform_spi_slave_config_t             wiced_spi_slave_config_t;

typedef platform_spi_slave_transfer_direction_t wiced_spi_slave_transfer_direction_t;

typedef platform_spi_slave_transfer_status_t    wiced_spi_slave_transfer_status_t;

typedef platform_spi_slave_command_t            wiced_spi_slave_command_t;

typedef platform_spi_slave_data_buffer_t        wiced_spi_slave_data_buffer_t;

/******************************************************
 *                    Structures
 ******************************************************/

/**
 * Specifies details of an external I2C slave device which is connected to the WICED system
 */
typedef struct
{
    wiced_i2c_t                   port;          /** Which I2C peripheral of the platform to use for the I2C device being specified */
    uint16_t                      address;       /** the address of the device on the I2C bus */
    wiced_i2c_bus_address_width_t address_width; /** Indicates the number of bits that the slave device uses for addressing */
    uint8_t                       flags;         /** Flags that change the mode of operation for the I2C port See WICED/platform/include/platform_peripheral.h  I2C flags constants */
    wiced_i2c_speed_mode_t        speed_mode;    /* speed mode the device operates in */
} wiced_i2c_device_t;


/**
 * Specifies details of an external SPI slave device which is connected to the WICED system
 */
typedef struct
{
    wiced_spi_t                   port;          /** Which SPI peripheral of the platform to use for the SPI device being specified */
    wiced_gpio_t                  chip_select;   /** Which hardware pin to use for Chip Select of the SPI device being specified */
    uint32_t                      speed;         /** SPI device access speed in Hertz */
    uint8_t                       mode;          /** Mode of operation for SPI port See WICED/platform/include/platform_peripheral.h  SPI mode constants */
    uint8_t                       bits;          /** Number of data bits - usually 8, 16 or 32 */
} wiced_spi_device_t;

/******************************************************
 *                     Variables
 ******************************************************/

#ifdef WICED_PLATFORM_INCLUDES_SPI_FLASH
extern const wiced_spi_device_t wiced_spi_flash;
#endif

/******************************************************
 * @endcond           Function Declarations
 ******************************************************/

/*****************************************************************************/
/** @defgroup platform       Platform functions
 *
 *  WICED hardware platform functions
 */
/*****************************************************************************/

/*****************************************************************************/
/** @addtogroup uart       UART
 *  @ingroup platform
 *
 * Universal Asynchronous Receiver Transmitter (UART) Functions
 *
 *
 *  @{
 */
/*****************************************************************************/

/** Initialises a UART interface
 *
 * Prepares an UART hardware interface for communications
 *
 * @param  uart     : the interface which should be initialised
 * @param  config   : UART configuration structure
 * @param  optional_rx_buffer : Pointer to an optional RX ring buffer
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_uart_init( wiced_uart_t uart, const wiced_uart_config_t* config, wiced_ring_buffer_t* optional_rx_buffer );


/** Deinitialises a UART interface
 *
 * @param  uart : the interface which should be deinitialised
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_uart_deinit( wiced_uart_t uart );


/** Transmit data on a UART interface
 *
 * @param  uart     : the UART interface
 * @param  data     : pointer to the start of data
 * @param  size     : number of bytes to transmit
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_uart_transmit_bytes( wiced_uart_t uart, const void* data, uint32_t size );


/** Receive data on a UART interface
 *
 * @param  uart     : the UART interface
 * @param  data     : pointer to the buffer which will store incoming data
 * @param  size     : number of bytes to receive, function return in same parameter number of actually received bytes
 * @param  timeout  : timeout in millisecond
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_uart_receive_bytes( wiced_uart_t uart, void* data, uint32_t * size, uint32_t timeout );

/** @} */
/*****************************************************************************/
/** @addtogroup spi       SPI
 *  @ingroup platform
 *
 * Serial Peripheral Interface (SPI) Functions
 *
 *
 *  @{
 */
/*****************************************************************************/

/** Initialises the SPI interface for a given SPI device
 *
 * Prepares a SPI hardware interface for communication as a master
 *
 * @param  spi : the SPI device to be initialised
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if the SPI device could not be initialised
 */
wiced_result_t wiced_spi_init( const wiced_spi_device_t* spi );


/** Transmits data to a SPI device
 *
 * @param  spi      : the SPI device to be initialised
 * @param  segments : a pointer to an array of segments
 * @param  number_of_segments : the number of segments to transfer
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_spi_transmit( const wiced_spi_device_t* spi, const wiced_spi_message_segment_t* segments, uint16_t number_of_segments );


/** Transmits and/or receives data from a SPI device
 *
 * @param  spi      : the SPI device to be initialised
 * @param  segments : a pointer to an array of segments
 * @param  number_of_segments : the number of segments to transfer
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_spi_transfer( const wiced_spi_device_t* spi, const wiced_spi_message_segment_t* segments, uint16_t number_of_segments );


/** De-initialises a SPI interface
 *
 * Turns off a SPI hardware interface
 *
 * @param  spi : the SPI device to be de-initialised
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_spi_deinit( const wiced_spi_device_t* spi );


/** Initialises a SPI slave interface
 *
 * @param[in]  spi    : the SPI slave interface to be initialised
 * @param[in]  config : SPI slave configuration
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_spi_slave_init( wiced_spi_t spi, const wiced_spi_slave_config_t* config );


/** De-initialises a SPI slave interface
 *
 * @param[in]  spi : the SPI slave interface to be de-initialised
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_spi_slave_deinit( wiced_spi_t spi );


/** Receive command from the remote SPI master
 *
 * @param[in]   spi         : the SPI slave interface
 * @param[out]  command     : pointer to the variable which will contained the received command
 * @param[in]   timeout_ms  : timeout in milliseconds
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_spi_slave_receive_command( wiced_spi_t spi, wiced_spi_slave_command_t* command, uint32_t timeout_ms );


/** Transfer data to/from the remote SPI master
 *
 * @param[in]  spi         : the SPI slave interface
 * @param[in]  direction   : transfer direction
 * @param[in]  buffer      : the buffer which contain the data to transfer
 * @param[in]  timeout_ms  : timeout in milliseconds
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_spi_slave_transfer_data( wiced_spi_t spi, wiced_spi_slave_transfer_direction_t direction, wiced_spi_slave_data_buffer_t* buffer, uint32_t timeout_ms );


/** Send an error status over the SPI slave interface
 *
 * @param[in]  spi          : the SPI slave interface
 * @param[in]  error_status : SPI slave error status
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_spi_slave_send_error_status( wiced_spi_t spi, wiced_spi_slave_transfer_status_t error_status );


/** Generate an interrupt on the SPI slave interface
 *
 * @param[in]  spi               : the SPI slave interface
 * @param[in]  pulse_duration_ms : interrupt pulse duration in milliseconds
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_spi_slave_generate_interrupt( wiced_spi_t spi, uint32_t pulse_duration_ms );


/** @} */
/*****************************************************************************/
/** @addtogroup i2c       I2C
 *  @ingroup platform
 *
 * Inter-IC bus (I2C) Functions
 *
 *
 *  @{
 */
/*****************************************************************************/

/** Initialises an I2C interface
 *
 * Prepares an I2C hardware interface for communication as a master
 *
 * @param  device : the device for which the i2c port should be initialised
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred during initialisation
 */
wiced_result_t wiced_i2c_init( const wiced_i2c_device_t* device );


/** Checks whether the device is available on a bus or not
 *
 *
 * @param  device : the i2c device to be probed
 * @param  retries    : the number of times to attempt to probe the device
 *
 * @return    WICED_TRUE : device is found.
 * @return    WICED_FALSE: device is not found
 */
wiced_bool_t wiced_i2c_probe_device( const wiced_i2c_device_t* device, int retries );


/** Initialize the wiced_i2c_message_t structure for i2c tx transaction
 *
 * @param message : pointer to a message structure, this should be a valid pointer
 * @param tx_buffer : pointer to a tx buffer that is already allocated
 * @param tx_buffer_length : number of bytes to transmit
 * @param retries    : the number of times to attempt send a message in case it can't not be sent
 * @param disable_dma : if true, disables the dma for current tx transaction. You may find it useful to switch off dma for short tx messages.
 *                     if you set this flag to 0, then you should make sure that the device flags was configured with I2C_DEVICE_USE_DMA. If the
 *                     device doesnt support DMA, the message will be transmitted with no DMA.
 *
 * @return    WICED_SUCCESS : message structure was initialised properly.
 * @return    WICED_BADARG: one of the arguments is given incorrectly
 */
wiced_result_t wiced_i2c_init_tx_message( wiced_i2c_message_t* message, const void* tx_buffer, uint16_t tx_buffer_length, uint16_t retries, wiced_bool_t disable_dma );


/** Initialize the wiced_i2c_message_t structure for i2c rx transaction
 *
 * @param message : pointer to a message structure, this should be a valid pointer
 * @param rx_buffer : pointer to an rx buffer that is already allocated
 * @param rx_buffer_length : number of bytes to receive
 * @param retries    : the number of times to attempt receive a message in case device doesnt respond
 * @param disable_dma : if true, disables the dma for current rx transaction. You may find it useful to switch off dma for short rx messages.
 *                     if you set this flag to 0, then you should make sure that the device flags was configured with I2C_DEVICE_USE_DMA. If the
 *                     device doesnt support DMA, the message will be received not using DMA.
 *
 * @return    WICED_SUCCESS : message structure was initialised properly.
 * @return    WICED_BADARG: one of the arguments is given incorrectly
 */
wiced_result_t wiced_i2c_init_rx_message( wiced_i2c_message_t* message, void* rx_buffer, uint16_t rx_buffer_length, uint16_t retries, wiced_bool_t disable_dma );


/** Initialize the wiced_i2c_message_t structure for i2c combined transaction
 *
 * @param  message : pointer to a message structure, this should be a valid pointer
 * @param tx_buffer: pointer to a tx buffer that is already allocated
 * @param rx_buffer: pointer to an rx buffer that is already allocated
 * @param tx_buffer_length: number of bytes to transmit
 * @param rx_buffer_length: number of bytes to receive
 * @param  retries    : the number of times to attempt receive a message in case device doesnt respond
 * @param disable_dma: if true, disables the dma for current rx transaction. You may find it useful to switch off dma for short rx messages.
 *                     if you set this flag to 0, then you should make sure that the device flags was configured with I2C_DEVICE_USE_DMA. If the
 *                     device doesnt support DMA, the message will be received not using DMA.
 *
 * @return    WICED_SUCCESS : message structure was initialised properly.
 * @return    WICED_BADARG: one of the arguments is given incorrectly
 */
wiced_result_t wiced_i2c_init_combined_message( wiced_i2c_message_t* message, const void* tx_buffer, void* rx_buffer, uint16_t tx_buffer_length, uint16_t rx_buffer_length, uint16_t retries, wiced_bool_t disable_dma );


/** Transmits and/or receives data over an I2C interface
 *
 * @param  device             : the i2c device to communicate with
 * @param  message            : a pointer to a message (or an array of messages) to be transmitted/received
 * @param  number_of_messages : the number of messages to transfer. [1 .. N] messages
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred during message transfer
 */
wiced_result_t wiced_i2c_transfer(  const wiced_i2c_device_t* device, wiced_i2c_message_t* message, uint16_t number_of_messages );


/** Read data over an I2C interface
 *
 * @param device             : the i2c device to communicate with
 * @param flags              : bitwise flags to control i2c data transfers (WICED_I2C_XXX_FLAG)
 * @param buffer             : pointer to a buffer to hold received data
 * @param buffer_length      : length in bytes of the buffer
 *
 * @return    WICED_SUCCESS : on success
 * @return    WICED_ERROR   : if an error occurred during message transfer
 */
wiced_result_t wiced_i2c_read( const wiced_i2c_device_t* device, uint16_t flags, void* buffer, uint16_t buffer_length );


/** Write data over an I2C interface
 *
 * @param device             : the i2c device to communicate with
 * @param flags              : bitwise flags to control i2c data transfers (WICED_I2C_XXX_FLAG)
 * @param buffer             : pointer to a buffer to hold received data
 * @param buffer_length      : length in bytes of the buffer
 *
 * @return    WICED_SUCCESS : on success
 * @return    WICED_ERROR   : if an error occurred during message transfer
 */
wiced_result_t wiced_i2c_write( const wiced_i2c_device_t* device, uint16_t flags, const void* buffer, uint16_t buffer_length );


/** Deinitialises an I2C device
 *
 * @param  device : the device for which the i2c port should be deinitialised
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred during deinitialisation
 */
wiced_result_t wiced_i2c_deinit( const wiced_i2c_device_t* device );

/** @} */
/*****************************************************************************/
/** @addtogroup adc       ADC
 *  @ingroup platform
 *
 * Analog to Digital Converter (ADC) Functions
 *
 *
 *  @{
 */
/*****************************************************************************/

/** Initialises an ADC interface
 *
 * Prepares an ADC hardware interface for sampling
 *
 * @param adc            : the interface which should be initialised
 * @param sampling_cycle : sampling period in number of ADC clock cycles. If the
 *                         MCU does not support the value provided, the closest
 *                         supported value is used.
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_adc_init( wiced_adc_t adc, uint32_t sampling_cycle );


/** Takes a single sample from an ADC interface
 *
 * Takes a single sample from an ADC interface
 *
 * @param adc    : the interface which should be sampled
 * @param output : pointer to a variable which will receive the sample
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_adc_take_sample( wiced_adc_t adc, uint16_t* output );


/** Takes multiple samples from an ADC interface
 *
 * Takes multiple samples from an ADC interface and stores them in
 * a memory buffer
 *
 * @param adc           : the interface which should be sampled
 * @param buffer        : a memory buffer which will receive the samples
 *                        Each sample will be uint16_t little endian.
 * @param buffer_length : length in bytes of the memory buffer.
 *
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_adc_take_sample_stream( wiced_adc_t adc, void* buffer, uint16_t buffer_length );


/** De-initialises an ADC interface
 *
 * Turns off an ADC hardware interface
 *
 * @param  adc : the interface which should be de-initialised
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_adc_deinit( wiced_adc_t adc );

/** @} */
/*****************************************************************************/
/** @addtogroup gpio       GPIO
 *  @ingroup platform
 *
 * General Purpose Input/Output pin (GPIO) Functions
 *
 *
 *  @{
 */
/*****************************************************************************/

/** Initialises a GPIO pin
 *
 * Prepares a GPIO pin for use.
 *
 * @param gpio          : the gpio pin which should be initialised
 * @param configuration : A structure containing the required
 *                        gpio configuration
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_gpio_init( wiced_gpio_t gpio, wiced_gpio_config_t configuration );

/** De-initialises a GPIO pin
 *
 * Clears a GPIO pin from use.
 *
 * @param gpio          : the gpio pin which should be de-initialised
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_gpio_deinit( wiced_gpio_t gpio );

/** Sets an output GPIO pin high
 *
 * Sets an output GPIO pin high. Using this function on a
 * gpio pin which is set to input mode is undefined.
 *
 * @param gpio          : the gpio pin which should be set high
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_gpio_output_high( wiced_gpio_t gpio );


/** Sets an output GPIO pin low
 *
 * Sets an output GPIO pin low. Using this function on a
 * gpio pin which is set to input mode is undefined.
 *
 * @param gpio          : the gpio pin which should be set low
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_gpio_output_low( wiced_gpio_t gpio );


/** Get the state of an input GPIO pin
 *
 * Get the state of an input GPIO pin. Using this function on a
 * gpio pin which is set to output mode will return an undefined value.
 *
 * @param gpio          : the gpio pin which should be read
 *
 * @return    WICED_TRUE  : if high
 * @return    WICED_FALSE : if low
 */
wiced_bool_t   wiced_gpio_input_get( wiced_gpio_t gpio );


/** Enables an interrupt trigger for an input GPIO pin
 *
 * Enables an interrupt trigger for an input GPIO pin.
 * Using this function on an uninitialized gpio pin or
 * a gpio pin which is set to output mode is undefined.
 *
 * @param gpio    : the gpio pin which will provide the interrupt trigger
 * @param trigger : the type of trigger (rising/falling edge, high/low level)
 * @param handler : a function pointer to the interrupt handler
 * @param arg     : an argument that will be passed to the
 *                  interrupt handler
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_gpio_input_irq_enable( wiced_gpio_t gpio, wiced_gpio_irq_trigger_t trigger, wiced_gpio_irq_handler_t handler, void* arg );


/** Disables an interrupt trigger for an input GPIO pin
 *
 * Disables an interrupt trigger for an input GPIO pin.
 * Using this function on a gpio pin which has not been set up
 * using @ref wiced_gpio_input_irq_enable is undefined.
 *
 * @param gpio    : the gpio pin which provided the interrupt trigger
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_gpio_input_irq_disable( wiced_gpio_t gpio );

/** @} */
/*****************************************************************************/
/** @addtogroup pwm       PWM
 *  @ingroup platform
 *
 * Pulse-Width Modulation (PWM) Functions
 *
 *
 *  @{
 */
/*****************************************************************************/

/** Initialises a PWM pin
 *
 * Prepares a Pulse-Width Modulation pin for use.
 * Does not start the PWM output (use @ref wiced_pwm_start).
 *
 * @param pwm        : the PWM interface which should be initialised
 * @param frequency  : Output signal frequency in Hertz
 * @param duty_cycle : Duty cycle of signal as a floating-point percentage (0.0 to 100.0)
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_pwm_init( wiced_pwm_t pwm, uint32_t frequency, float duty_cycle );


/** Starts PWM output on a PWM interface
 *
 * Starts Pulse-Width Modulation signal output on a PWM pin
 *
 * @param pwm        : the PWM interface which should be started
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_pwm_start( wiced_pwm_t pwm );


/** Stops output on a PWM pin
 *
 * Stops Pulse-Width Modulation signal output on a PWM pin
 *
 * @param pwm        : the PWM interface which should be stopped
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_pwm_stop( wiced_pwm_t pwm );

/** @} */
/*****************************************************************************/
/** @addtogroup watchdog       Watchdog
 *  @ingroup platform
 *
 * Watchdog Timer Functions
 *
 *
 *  @{
 */
/*****************************************************************************/

/** Kick the system watchdog.
 *
 * Resets (kicks) the timer of the system watchdog.
 * This MUST be done done by the App regularly.
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_watchdog_kick( void );

/** @} */
/*****************************************************************************/
/** @addtogroup mcupowersave       Powersave
 *  @ingroup platform
 *
 * MCU Powersave Functions
 *
 *
 *  @{
 */
/*****************************************************************************/

/** Enables the MCU to enter powersave mode.
 *
 * @warning   If the MCU drives the sleep clock input pin of the WLAN chip,   \n
 *            ensure the 32kHz clock output from the MCU is maintained while  \n
 *            the MCU is in powersave mode. The WLAN sleep clock reference is \n
 *            typically configured in the file:                               \n
 *            <WICED-SDK>/include/platforms/<PLATFORM_NAME>/platform.h
 * @return    void
 */
void wiced_platform_mcu_enable_powersave( void );


/** Stops the MCU entering powersave mode.
 *
 * @return    void
 */
void wiced_platform_mcu_disable_powersave( void );

/** @} */

/**
 * This function will return the value of time read from the on board CPU real time clock. Time value must be given in the format of
 * the structure wiced_rtc_time_t
 *
 * @param time        : pointer to a time structure
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_platform_get_rtc_time( wiced_rtc_time_t* time );


/**
 * This function will set MCU RTC time to a new value. Time value must be given in the format of
 * the structure wiced_rtc_time_t
 *
 * @param time        : pointer to a time structure
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_platform_set_rtc_time( const wiced_rtc_time_t* time );


/**
 * Enable the 802.1AS time functionality.
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_time_enable_8021as(void);


/**
 * Disable the 802.1AS time functionality.
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_time_disable_8021as(void);


/**
 * Read the 802.1AS time.
 *
 * Retrieve the origin timestamp in the last sync message, correct for the
 * intervening interval and return the corrected time in seconds + nanoseconds.
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_time_read_8021as(uint32_t *master_secs, uint32_t *master_nanosecs,
                                      uint32_t *local_secs, uint32_t *local_nanosecs);


/**
 * Read the 802.1AS time along with the I2S-driven audio time
 *
 * Retrieve the origin timestamp in the last sync message, correct for the
 * intervening interval and return the corrected time in seconds + nanoseconds.
 * Also retrieve the corresponding time from the audio timer.
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_time_read_8021as_with_audio(uint32_t *master_secs, uint32_t *master_nanosecs,
                                                 uint32_t *local_secs, uint32_t *local_nanosecs,
                                                 uint32_t *audio_time_hi, uint32_t *audio_time_lo);


/**
 * Enable audio timer
 *
 * @param     audio_frame_count : audio timer interrupts period expressed in number of audio samples/frames
 *
 * @return    WICED_SUCCESS     : on success.
 * @return    WICED_ERROR       : if an error occurred with any step
 */
wiced_result_t wiced_audio_timer_enable        ( uint32_t audio_frame_count );


/**
 * Disable audio timer
 *
 * @return    WICED_SUCCESS     : on success.
 * @return    WICED_ERROR       : if an error occurred with any step
 */
wiced_result_t wiced_audio_timer_disable       ( void );


/**
 * Wait for audio timer frame sync event
 *
 * @param     timeout_msecs     : timeout value in msecs; WICED_NO_WAIT or WICED_WAIT_FOREVER otherwise.
 *
 * @return    WICED_SUCCESS     : on success.
 * @return    WICED_ERROR       : if an error occurred with any step
 */
wiced_result_t wiced_audio_timer_get_frame_sync( uint32_t timeout_msecs );


/**
 * Read audio timer value (tick count)
 *
 * @param     time_hi           : upper 32-bit of 64-bit audio timer ticks
 * @param     time_lo           : lower 32-bit of 64-bit audio timer ticks
 *
 * @return    WICED_SUCCESS     : on success.
 * @return    WICED_ERROR       : if an error occurred with any step
 */
wiced_result_t wiced_audio_timer_get_time      ( uint32_t *time_hi, uint32_t *time_lo );


/**
 * Get audio timer resolution (ticks per second)
 *
 * @param     audio_sample_rate : audio sample rate
 * @param     ticks_per_sec     : returned audio timer resolution
 *
 * @return    WICED_SUCCESS     : on success.
 * @return    WICED_ERROR       : if an error occurred with any step
 */
wiced_result_t wiced_audio_timer_get_resolution( uint32_t audio_sample_rate, uint32_t *ticks_per_sec );

#ifdef __cplusplus
} /*extern "C" */
#endif
