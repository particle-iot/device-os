/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *  Broadcom WLAN SDIO Protocol interface
 *
 *  Implements the WWD Bus Protocol Interface for SDIO
 *  Provides functions for initialising, de-intitialising 802.11 device,
 *  sending/receiving raw packets etc
 */


#include <string.h> /* For memcpy */
#include "wwd_assert.h"
#include "network/wwd_buffer_interface.h"
#include "RTOS/wwd_rtos_interface.h"
#include "platform/wwd_platform_interface.h"
#include "chip_constants.h"
#include "internal/wwd_internal.h"
#include "internal/wwd_sdpcm.h"
#include "internal/bus_protocols/wwd_bus_protocol_interface.h"

/******************************************************
 *             Constants
 ******************************************************/

#define F0_WORKING_TIMEOUT_MS (500)
#define F1_AVAIL_TIMEOUT_MS   (500)
#define F2_AVAIL_TIMEOUT_MS   (500)
#define F2_READY_TIMEOUT_MS   (1000)
#define ALP_AVAIL_TIMEOUT_MS  (100)
#define HT_AVAIL_TIMEOUT_MS   (500)
#define ABORT_TIMEOUT_MS      (100)
/* Taken from FALCON_5_90_195_26 dhd/sys/dhd_sdio.c. */
#define SDIO_F2_WATERMARK     (8)

#define WLAN_BUS_UP_ATTEMPTS  (1000)

#define INITIAL_READ   4

#define BUS_PROTOCOL_LEVEL_MAX_RETRIES   5

#define VERIFY_RESULT( x )  { wwd_result_t verify_result; verify_result = (x); if ( verify_result != WWD_SUCCESS ) { wiced_assert("command failed",0==1); return verify_result; } }

#ifndef WWD_BUS_SDIO_RESET_DELAY
#define WWD_BUS_SDIO_RESET_DELAY    (1)
#endif

#ifndef WWD_BUS_SDIO_AFTER_RESET_DELAY
#define WWD_BUS_SDIO_AFTER_RESET_DELAY    (1)
#endif

/******************************************************
 *             Structures
 ******************************************************/

#pragma pack(1)
typedef struct
{
    unsigned char stuff_bits;
    unsigned int  ocr :24;
} sdio_cmd5_argument_t;

typedef struct
{
    unsigned int  _unique2         : 9; /* 0-8   */
    unsigned int  register_address :17; /* 9-25  */
    unsigned int  _unique          : 2; /* 26-27 */
    unsigned int  function_number  : 3; /* 28-30 */
    unsigned int  rw_flag          : 1; /* 31    */
} sdio_cmd5x_argument_t;

typedef struct
{
    uint8_t       write_data;           /* 0 - 7 */
    unsigned int  _stuff2          : 1; /* 8     */
    unsigned int  register_address :17; /* 9-25  */
    unsigned int  _stuff           : 1; /* 26    */
    unsigned int  raw_flag         : 1; /* 27    */
    unsigned int  function_number  : 3; /* 28-30 */
    unsigned int  rw_flag          : 1; /* 31    */
} wwd_bus_sdio_cmd52_argument_t;

typedef struct
{
    unsigned int  count            : 9; /* 0-8   */
    unsigned int  register_address :17; /* 9-25  */
    unsigned int  op_code          : 1; /* 26    */
    unsigned int  block_mode       : 1; /* 27    */
    unsigned int  function_number  : 3; /* 28-30 */
    unsigned int  rw_flag          : 1; /* 31    */
} wwd_bus_sdio_cmd53_argument_t;

typedef union
{
    uint32_t              value;
    sdio_cmd5_argument_t  cmd5;
    sdio_cmd5x_argument_t cmd5x;
    wwd_bus_sdio_cmd52_argument_t cmd52;
    wwd_bus_sdio_cmd53_argument_t cmd53;
} sdio_cmd_argument_t;

typedef struct
{
    unsigned int  ocr              :24; /* 0-23  */
    unsigned int  stuff_bits       : 3; /* 24-26 */
    unsigned int  memory_present   : 1; /* 27    */
    unsigned int  function_count   : 3; /* 28-30 */
    unsigned int  c                : 1; /* 31    */
} sdio_response4_t;

typedef struct
{
    uint8_t       data;                /* 0-7   */
    uint8_t       response_flags;       /* 8-15  */
    uint16_t      stuff;               /* 16-31 */
} sdio_response5_t;

typedef struct
{
    uint16_t      card_status;          /* 0-15  */
    uint16_t      rca;                 /* 16-31 */
} sdio_response6_t;

typedef union
{
    uint32_t                    value;
    sdio_response4_t            r4;
    sdio_response5_t            r5;
    sdio_response6_t            r6;
} sdio_response_t;

#pragma pack()

/******************************************************
 *             Variables
 ******************************************************/

static wiced_bool_t bus_is_up               = WICED_FALSE;
static wiced_bool_t wwd_bus_flow_controlled = WICED_FALSE;

/******************************************************
 *             Static Function Declarations
 ******************************************************/

static wwd_result_t wwd_bus_sdio_transfer               ( wwd_bus_transfer_direction_t direction, wwd_bus_function_t function, uint32_t address, uint16_t data_size, /*@in@*/ /*@out@*/  uint8_t* data, sdio_response_needed_t response_expected );
static wwd_result_t wwd_bus_sdio_cmd52                  ( wwd_bus_transfer_direction_t direction, wwd_bus_function_t function, uint32_t address, uint8_t value, sdio_response_needed_t response_expected, /*@out@*/ uint8_t* response );
static wwd_result_t wwd_bus_sdio_cmd53                  ( wwd_bus_transfer_direction_t direction, wwd_bus_function_t function, sdio_transfer_mode_t mode, uint32_t address, uint16_t data_size, /*@in@*/ /*@out@*/  uint8_t* data, sdio_response_needed_t response_expected, /*@null@*/ /*@out@*/ uint32_t* response );
static wwd_result_t wwd_bus_sdio_abort_read             ( wiced_bool_t retry );
static wwd_result_t wwd_bus_sdio_download_firmware      ( void );
static wwd_result_t wwd_bus_sdio_read_register_value    ( wwd_bus_function_t function, uint32_t address, uint8_t value_length, /*@out@*/ uint8_t* value );

#ifndef WICED_DISABLE_MCU_POWERSAVE
static wwd_result_t wwd_bus_sdio_set_oob_interrupt      ( uint8_t gpio_pin_number );
#endif

/******************************************************
 *             SDIO Logging
 * Enable this section for logging of SDIO transfers
 * by changing "if 0" to "if 1"
 ******************************************************/
#if 0

#define SDIO_LOG_SIZE (110)
#define SDIO_LOG_HEADER_SIZE (0)   /*(0x30) */

typedef struct sdio_log_entry_struct
{
    wwd_bus_transfer_direction_t  direction;
    wwd_bus_function_t            function;
    uint32_t                  address;
    unsigned long             time;
    unsigned long             length;
#if ( SDIO_LOG_HEADER_SIZE != 0 )
    unsigned char             header[SDIO_LOG_HEADER_SIZE];
#endif /* if ( SDIO_LOG_HEADER_SIZE != 0 ) */
} sdio_log_entry_t;

static int next_sdio_log_pos = 0;
static sdio_log_entry_t sdio_log_data[SDIO_LOG_SIZE];

static void add_log_entry( wwd_bus_transfer_direction_t dir, wwd_bus_function_t function, uint32_t address, unsigned long length, uint8_t* data )
{
    sdio_log_data[next_sdio_log_pos].direction = dir;
    sdio_log_data[next_sdio_log_pos].function  = function;
    sdio_log_data[next_sdio_log_pos].address   = address;
    sdio_log_data[next_sdio_log_pos].time      = host_rtos_get_time();
    sdio_log_data[next_sdio_log_pos].length    = length;
#if ( SDIO_LOG_HEADER_SIZE != 0 )
    memcpy( sdio_log_data[next_sdio_log_pos].header, data, (length>=SDIO_LOG_HEADER_SIZE)?SDIO_LOG_HEADER_SIZE:length );
#else
    UNUSED_PARAMETER(data);
#endif /* if ( SDIO_LOG_HEADER_SIZE != 0 ) */
    next_sdio_log_pos++;
    if (next_sdio_log_pos >= SDIO_LOG_SIZE)
    {
        next_sdio_log_pos = 0;
    }
}
#else /* #if 0 */
#define add_log_entry( dir, function, address, length, data)
#endif /* #if 0 */


/******************************************************
 *             Global Function definitions
 ******************************************************/

/* Device data transfer functions */
wwd_result_t wwd_bus_send_buffer( wiced_buffer_t buffer )
{
    wwd_result_t retval;
    retval = wwd_bus_transfer_bytes( BUS_WRITE, WLAN_FUNCTION, 0, (uint16_t) ( host_buffer_get_current_piece_size( buffer ) - sizeof(wiced_buffer_t) ), (wwd_transfer_bytes_packet_t*)( host_buffer_get_current_piece_data_pointer( buffer ) + sizeof(wiced_buffer_t) ) );
    host_buffer_release( buffer, WWD_NETWORK_TX );
    return retval;
}

wwd_result_t wwd_bus_init( void )
{
    uint8_t        byte_data;
    wwd_result_t   result;
    uint32_t       loop_count;

    wwd_bus_flow_controlled = WICED_FALSE;

    wwd_bus_init_backplane_window( );

    host_platform_reset_wifi( WICED_TRUE );
    host_platform_power_wifi( WICED_TRUE );
    (void) host_rtos_delay_milliseconds( (uint32_t) WWD_BUS_SDIO_RESET_DELAY );  /* Ignore return - nothing can be done if it fails */
    host_platform_reset_wifi( WICED_FALSE );
    (void) host_rtos_delay_milliseconds( (uint32_t) WWD_BUS_SDIO_AFTER_RESET_DELAY );  /* Ignore return - nothing can be done if it fails */

    VERIFY_RESULT( host_platform_sdio_enumerate() );

    /* Setup the backplane*/
    loop_count = 0;
    do
    {
        /* Enable function 1 (backplane) */
        VERIFY_RESULT( wwd_bus_write_register_value( BUS_FUNCTION, SDIOD_CCCR_IOEN, (uint8_t) 1, SDIO_FUNC_ENABLE_1 ) );
        if (loop_count != 0)
        {
            (void) host_rtos_delay_milliseconds( (uint32_t) 1 );  /* Ignore return - nothing can be done if it fails */
        }
        VERIFY_RESULT( wwd_bus_sdio_read_register_value ( BUS_FUNCTION, SDIOD_CCCR_IOEN, (uint8_t) 1, &byte_data ) );
        loop_count++;
        if ( loop_count >= (uint32_t) F0_WORKING_TIMEOUT_MS )
        {
            return WWD_TIMEOUT;
        }
    } while (byte_data != (uint8_t) SDIO_FUNC_ENABLE_1);

#ifndef SDIO_1_BIT
    /* Read the bus width and set to 4 bits */
    VERIFY_RESULT( wwd_bus_sdio_read_register_value (BUS_FUNCTION, SDIOD_CCCR_BICTRL, (uint8_t) 1, &byte_data) );
    VERIFY_RESULT( wwd_bus_write_register_value(BUS_FUNCTION, SDIOD_CCCR_BICTRL, (uint8_t) 1, (byte_data & (~BUS_SD_DATA_WIDTH_MASK)) | BUS_SD_DATA_WIDTH_4BIT ) );
    /* NOTE: We don't need to change our local bus settings since we're not sending any data (only using CMD52) until after we change the bus speed further down */
#endif
    /* Set the block size */

    /* Wait till the backplane is ready */
    loop_count = 0;
    while ( ( ( result = wwd_bus_write_register_value( BUS_FUNCTION, SDIOD_CCCR_BLKSIZE_0, (uint8_t) 1, (uint32_t) SDIO_64B_BLOCK ) ) == WWD_SUCCESS ) &&
            ( ( result = wwd_bus_sdio_read_register_value ( BUS_FUNCTION, SDIOD_CCCR_BLKSIZE_0, (uint8_t) 1, &byte_data                ) ) == WWD_SUCCESS ) &&
            ( byte_data != (uint8_t)  SDIO_64B_BLOCK ) &&
            ( loop_count < (uint32_t) F0_WORKING_TIMEOUT_MS ) )
    {
        (void) host_rtos_delay_milliseconds( (uint32_t) 1 );  /* Ignore return - nothing can be done if it fails */
        loop_count++;
        if ( loop_count >= (uint32_t) F0_WORKING_TIMEOUT_MS )
        {
            /* If the system fails here, check the high frequency crystal is working */
            return WWD_TIMEOUT;
        }
    }

    VERIFY_RESULT( result );

    VERIFY_RESULT( wwd_bus_write_register_value( BUS_FUNCTION, SDIOD_CCCR_BLKSIZE_0,   (uint8_t) 1, (uint32_t) SDIO_64B_BLOCK ) );
    VERIFY_RESULT( wwd_bus_write_register_value( BUS_FUNCTION, SDIOD_CCCR_F1BLKSIZE_0, (uint8_t) 1, (uint32_t) SDIO_64B_BLOCK ) );
    VERIFY_RESULT( wwd_bus_write_register_value( BUS_FUNCTION, SDIOD_CCCR_F2BLKSIZE_0, (uint8_t) 1, (uint32_t) SDIO_64B_BLOCK ) );
    VERIFY_RESULT( wwd_bus_write_register_value( BUS_FUNCTION, SDIOD_CCCR_F2BLKSIZE_1, (uint8_t) 1, (uint32_t) 0              ) ); /* Function 2 = 64 */

    /* Enable/Disable Client interrupts */
    VERIFY_RESULT( wwd_bus_write_register_value( BUS_FUNCTION, SDIOD_CCCR_INTEN,       (uint8_t) 1, INTR_CTL_MASTER_EN | INTR_CTL_FUNC1_EN | INTR_CTL_FUNC2_EN ) );


#if 0
    /* This code is required if we want more than 25 MHz clock */
    VERIFY_RESULT( wwd_bus_sdio_read_register_value( BUS_FUNCTION, SDIOD_CCCR_SPEED_CONTROL, 1, &byte_data ) );
    if ( ( byte_data & 0x1 ) != 0 )
    {
        VERIFY_RESULT( wwd_bus_write_register_value( BUS_FUNCTION, SDIOD_CCCR_SPEED_CONTROL, 1, byte_data | SDIO_SPEED_EHS ) );
    }
    else
    {
        return WWD_BUS_READ_REGISTER_ERROR;
    }
#endif

    /* Switch to high speed mode and change to 4 bit mode */
    host_platform_enable_high_speed_sdio( );

    /* Wait till the backplane is ready */
    loop_count = 0;
    while ( ( ( result = wwd_bus_sdio_read_register_value( BUS_FUNCTION, SDIOD_CCCR_IORDY, (uint8_t) 1, &byte_data ) ) == WWD_SUCCESS ) &&
            ( ( byte_data & SDIO_FUNC_READY_1 ) == 0 ) &&
            ( loop_count < (uint32_t) F1_AVAIL_TIMEOUT_MS ) )
    {
        (void) host_rtos_delay_milliseconds( (uint32_t) 1 ); /* Ignore return - nothing can be done if it fails */
        loop_count++;
    }
    if ( loop_count >= (uint32_t) F1_AVAIL_TIMEOUT_MS )
    {
        return WWD_TIMEOUT;
    }
    VERIFY_RESULT( result );

    /* Set the ALP */
    VERIFY_RESULT( wwd_bus_write_register_value( BACKPLANE_FUNCTION, SDIO_CHIP_CLOCK_CSR, (uint8_t) 1, (uint32_t)( SBSDIO_FORCE_HW_CLKREQ_OFF | SBSDIO_ALP_AVAIL_REQ | SBSDIO_FORCE_ALP ) ) );

    loop_count = 0;
    while ( ( ( result = wwd_bus_sdio_read_register_value( BACKPLANE_FUNCTION, SDIO_CHIP_CLOCK_CSR, (uint8_t) 1, &byte_data ) ) == WWD_SUCCESS ) &&
            ( ( byte_data & SBSDIO_ALP_AVAIL ) == 0 ) &&
            ( loop_count < (uint32_t) ALP_AVAIL_TIMEOUT_MS ) )
    {
        (void) host_rtos_delay_milliseconds( (uint32_t) 1 ); /* Ignore return - nothing can be done if it fails */
        loop_count++;
    }
    if ( loop_count >= (uint32_t) ALP_AVAIL_TIMEOUT_MS )
    {
        return WWD_TIMEOUT;
    }
    VERIFY_RESULT( result );

    /* Clear request for ALP */
    VERIFY_RESULT( wwd_bus_write_register_value( BACKPLANE_FUNCTION, SDIO_CHIP_CLOCK_CSR, (uint8_t) 1, 0 ) );

    /* Disable the extra SDIO pull-ups */
#ifndef WWD_BUS_SDIO_USE_WLAN_SDIO_PULLUPS
    VERIFY_RESULT( wwd_bus_write_register_value( BACKPLANE_FUNCTION, SDIO_PULL_UP,  (uint8_t) 1, 0 ) );
#endif

    /* Enable F1 and F2 */
    VERIFY_RESULT( wwd_bus_write_register_value( BUS_FUNCTION, SDIOD_CCCR_IOEN, (uint8_t) 1, SDIO_FUNC_ENABLE_1 | SDIO_FUNC_ENABLE_2 ) );

#ifndef WICED_DISABLE_MCU_POWERSAVE
    /* Enable out-of-band interrupt */
    VERIFY_RESULT( wwd_bus_write_register_value( BUS_FUNCTION, SDIOD_SEP_INT_CTL, (uint8_t) 1, SEP_INTR_CTL_MASK | SEP_INTR_CTL_EN | SEP_INTR_CTL_POL ) );

    /* Set OOB interrupt to the correct WLAN GPIO pin */
    VERIFY_RESULT( wwd_bus_sdio_set_oob_interrupt( host_platform_get_oob_interrupt_pin( ) ) );

    VERIFY_RESULT( host_enable_oob_interrupt( ) );
#endif /* ifndef WICED_DISABLE_MCU_POWERSAVE */

    /* Enable F2 interrupt only */
    VERIFY_RESULT( wwd_bus_write_register_value( BUS_FUNCTION, SDIOD_CCCR_INTEN, (uint8_t) 1, INTR_CTL_MASTER_EN | INTR_CTL_FUNC2_EN ) );

    VERIFY_RESULT( wwd_bus_sdio_read_register_value( BUS_FUNCTION, SDIOD_CCCR_IORDY, (uint8_t) 1, &byte_data ) );

    VERIFY_RESULT( wwd_bus_sdio_download_firmware( ) );

    /* Wait for F2 to be ready */
    loop_count = 0;
    while ( ( ( result = wwd_bus_sdio_read_register_value( BUS_FUNCTION, SDIOD_CCCR_IORDY, (uint8_t) 1, &byte_data ) ) == WWD_SUCCESS ) &&
            ( ( byte_data & SDIO_FUNC_READY_2 ) == 0 ) &&
            ( loop_count < (uint32_t) F2_READY_TIMEOUT_MS ) )
    {
        (void) host_rtos_delay_milliseconds( (uint32_t) 1 ); /* Ignore return - nothing can be done if it fails */
        loop_count++;
    }
    if ( loop_count >= (uint32_t) F2_READY_TIMEOUT_MS )
    {
        /* If your system fails here, it could be due to incorrect NVRAM variables.
         * Check which 'wifi_nvram_image.h' file your platform is using, and
         * check that it matches the WLAN device on your platform, including the
         * crystal frequency.
         */
        WPRINT_WWD_ERROR(("Timeout while waiting for function 2 to be ready\n"));
        /*@-unreachable@*/ /* Reachable after hitting assert */
        return WWD_TIMEOUT;
        /*@+unreachable@*/
    }

    bus_is_up = WICED_TRUE;

    return result;
}

wwd_result_t wwd_bus_deinit( void )
{
    /* put device in reset. */
    host_platform_reset_wifi( WICED_TRUE );

    bus_is_up = WICED_FALSE;

    return WWD_SUCCESS;
}

wwd_result_t wwd_bus_ack_interrupt(uint32_t intstatus)
{
    return wwd_bus_write_backplane_value( (uint32_t) SDIO_INT_STATUS, (uint8_t) 4, intstatus);
}

uint32_t wwd_bus_packet_available_to_read(void)
{
    uint32_t int_status = 0;

    /* Read the IntStatus */
    if ( wwd_bus_read_backplane_value( (uint32_t) SDIO_INT_STATUS, (uint8_t) 4, (uint8_t*)&int_status ) != WWD_SUCCESS )
    {
    }

    /* Clear any interrupts */
    if ( wwd_bus_write_backplane_value( (uint32_t) SDIO_INT_STATUS, (uint8_t) 4, int_status ) != WWD_SUCCESS )
    {
    }

#if defined(WICED_PLATFORM_MASKS_BUS_IRQ)
    host_platform_unmask_sdio_interrupt();
#endif

    return ((int_status) & (FRAME_AVAILABLE_MASK));
}

/*
 * From internal documentation: hwnbu-twiki/SdioMessageEncapsulation
 * When data is available on the device, the device will issue an interrupt:
 * - the device should signal the interrupt as a hint that one or more data frames may be available on the device for reading
 * - the host may issue reads of the 4 byte length tag at any time -- that is, whether an interupt has been issued or not
 * - if a frame is available, the tag read should return a nonzero length (>= 4) and the host can then read the remainder of the frame by issuing one or more CMD53 reads
 * - if a frame is not available, the 4byte tag read should return zero
 */
wwd_result_t wwd_bus_read_frame( /*@out@*/ wiced_buffer_t* buffer )
{
    uint16_t hwtag[8];
    uint16_t extra_space_required;
    wwd_result_t result;

    *buffer = NULL;

    /* Ensure the wlan backplane bus is up */
    VERIFY_RESULT( wwd_bus_ensure_is_up() );

    /* Read the frame header and verify validity */
    memset( hwtag, 0, sizeof(hwtag) );

    result = wwd_bus_sdio_transfer(BUS_READ, WLAN_FUNCTION, 0, (uint16_t) INITIAL_READ, (uint8_t*)hwtag, RESPONSE_NEEDED);
    if ( result != WWD_SUCCESS )
    {
        (void) wwd_bus_sdio_abort_read( WICED_FALSE ); /* ignore return - not much can be done if this fails */
        return WWD_SDIO_RX_FAIL;
    }

    if ( ( ( hwtag[0] | hwtag[1] ) == 0                 ) ||
         ( ( hwtag[0] ^ hwtag[1] ) != (uint16_t) 0xFFFF ) )
    {
        return WWD_HWTAG_MISMATCH;
    }

    if ( ( hwtag[0] == (uint16_t) 12 ) &&
         ( wwd_wlan_status.state == WLAN_UP) )
    {
        result = wwd_bus_sdio_transfer(BUS_READ, WLAN_FUNCTION, 0, (uint16_t) 8, (uint8_t*) &hwtag[2], RESPONSE_NEEDED);
        if ( result != WWD_SUCCESS )
        {
            (void) wwd_bus_sdio_abort_read( WICED_FALSE ); /* ignore return - not much can be done if this fails */
            return WWD_SDIO_RX_FAIL;
        }
        wwd_sdpcm_update_credit((uint8_t*)hwtag);
        return WWD_SUCCESS;
    }

    /* Calculate the space we need to store entire packet */
    if ( ( hwtag[0] > (uint16_t) INITIAL_READ ) )
    {
        extra_space_required = (uint16_t) ( hwtag[0] - (uint16_t) INITIAL_READ );
    }
    else
    {
        extra_space_required = 0;
    }

    /* Allocate a suitable buffer */
    result = host_buffer_get( buffer, WWD_NETWORK_RX, (unsigned short) ( (uint16_t) INITIAL_READ + extra_space_required + (uint16_t) sizeof(wwd_buffer_header_t) ), WICED_FALSE );
    if ( result != WWD_SUCCESS )
    {
        /* Read out the first 12 bytes to get the bus credit information, 4 bytes are already read in hwtag */
        wiced_assert( "Get buffer error", ( ( result == WWD_BUFFER_UNAVAILABLE_TEMPORARY ) || ( result == WWD_BUFFER_UNAVAILABLE_PERMANENT ) ) );
        result = wwd_bus_sdio_transfer(BUS_READ, WLAN_FUNCTION, 0, (uint16_t) 8, (uint8_t*) &hwtag[2], RESPONSE_NEEDED);
        if ( result != WWD_SUCCESS )
        {
            (void) wwd_bus_sdio_abort_read( WICED_FALSE ); /* ignore return - not much can be done if this fails */
            return WWD_SDIO_RX_FAIL;
        }
        result = wwd_bus_sdio_abort_read( WICED_FALSE );
        wiced_assert( "Read-abort failed", result==WWD_SUCCESS );
        wwd_sdpcm_update_credit( (uint8_t *)hwtag );
        return WWD_RX_BUFFER_ALLOC_FAIL;
    }

    /* Copy the data already read */
    memset( host_buffer_get_current_piece_data_pointer( *buffer ) + sizeof(wwd_buffer_header_t), 0, ( size_t )( INITIAL_READ + extra_space_required ) );
    memcpy( host_buffer_get_current_piece_data_pointer( *buffer ) + sizeof(wwd_buffer_header_t), hwtag, (size_t) INITIAL_READ );

    /* Read the rest of the data */
    if ( extra_space_required > 0 )
    {
        result = wwd_bus_sdio_transfer(BUS_READ, WLAN_FUNCTION, 0, extra_space_required, host_buffer_get_current_piece_data_pointer( *buffer ) + sizeof(wwd_buffer_header_t) + INITIAL_READ, RESPONSE_NEEDED);
        if ( result != WWD_SUCCESS )
        {
            (void) wwd_bus_sdio_abort_read( WICED_FALSE ); /* ignore return - not much can be done if this fails */
            host_buffer_release(*buffer, WWD_NETWORK_RX);
            return WWD_SDIO_RX_FAIL;
        }
    }

    return WWD_SUCCESS;
}


/******************************************************
 *     Function definitions for Protocol Common
 ******************************************************/

/* Device register access functions */
wwd_result_t wwd_bus_write_backplane_value( uint32_t address, uint8_t register_length, uint32_t value )
{
    VERIFY_RESULT( wwd_bus_set_backplane_window( address ) );

    return wwd_bus_sdio_transfer( BUS_WRITE, BACKPLANE_FUNCTION, address & 0x07FFF, register_length, (uint8_t*) &value, RESPONSE_NEEDED );
}

wwd_result_t wwd_bus_read_backplane_value( uint32_t address, uint8_t register_length, /*@out@*/ uint8_t* value )
{
    *value = 0;
    VERIFY_RESULT( wwd_bus_set_backplane_window( address ) );

    return wwd_bus_sdio_transfer( BUS_READ, BACKPLANE_FUNCTION, address & 0x07FFF, register_length, value, RESPONSE_NEEDED );
}

wwd_result_t wwd_bus_write_register_value( wwd_bus_function_t function, uint32_t address, uint8_t value_length, uint32_t value )
{
    return wwd_bus_sdio_transfer( BUS_WRITE, function, address, value_length, (uint8_t*) &value, RESPONSE_NEEDED );
}

wwd_result_t wwd_bus_transfer_bytes( wwd_bus_transfer_direction_t direction, wwd_bus_function_t function, uint32_t address, uint16_t size, /*@in@*/ /*@out@*/ wwd_transfer_bytes_packet_t* data )
{
    return wwd_bus_sdio_transfer( direction, function, address, size, (uint8_t*)data, RESPONSE_NEEDED );
}


/******************************************************
 *             Static  Function definitions
 ******************************************************/

static wwd_result_t wwd_bus_sdio_transfer( wwd_bus_transfer_direction_t direction, wwd_bus_function_t function, uint32_t address, uint16_t data_size, /*@in@*/ /*@out@*/ uint8_t* data, sdio_response_needed_t response_expected )
{
    wwd_result_t result;
    uint8_t retry_count = 0;
    do
    {
        if ( data_size == (uint16_t) 1 )
        {
            result = wwd_bus_sdio_cmd52( direction, function, address, *data, response_expected, data );
        }
        else
        {
            result = wwd_bus_sdio_cmd53( direction, function, ( data_size >= (uint16_t) 64 ) ? SDIO_BLOCK_MODE : SDIO_BYTE_MODE, address, data_size, data, response_expected, NULL );
        }

        if ( result != WWD_SUCCESS )
        {
            return result;
#if 0
            if (function == WLAN_FUNCTION && direction == BUS_READ)
            {
                (void) wwd_bus_sdio_abort_read( WICED_TRUE ); /* Ignore return as there is not much to be done if this fails */
                wiced_assert("Attempting to abort read after transfer failure - may not fix bus", 0 != 0 );
            }

            ++retry_count;
#endif /* if 0 */
        }
    } while ( ( result != WWD_SUCCESS ) && ( retry_count < (uint8_t) BUS_PROTOCOL_LEVEL_MAX_RETRIES ) );

    return result;
}

static wwd_result_t wwd_bus_sdio_cmd52( wwd_bus_transfer_direction_t direction, wwd_bus_function_t function, uint32_t address, uint8_t value, sdio_response_needed_t response_expected, uint8_t* response )
{
    uint32_t sdio_response;
    wwd_result_t result;
    sdio_cmd_argument_t arg;
    arg.value = 0;
    arg.cmd52.function_number  = (unsigned int) ( function & BUS_FUNCTION_MASK );
    arg.cmd52.register_address = (unsigned int) ( address & 0x00001ffff );
    arg.cmd52.rw_flag = (unsigned int) ( ( direction == BUS_WRITE ) ? 1 : 0 );
    arg.cmd52.write_data = value;
    result = host_platform_sdio_transfer( direction, SDIO_CMD_52, SDIO_BYTE_MODE, SDIO_1B_BLOCK, arg.value, 0, 0, response_expected, &sdio_response );
    if ( response != NULL )
    {
        *response = (uint8_t) ( sdio_response & 0x00000000ff );
    }
    return result;
}

static wwd_result_t wwd_bus_sdio_cmd53( wwd_bus_transfer_direction_t direction, wwd_bus_function_t function, sdio_transfer_mode_t mode, uint32_t address, uint16_t data_size, /*@in@*/ /*@out@*/ uint8_t* data, sdio_response_needed_t response_expected, /*@null@*/ uint32_t* response )
{
    sdio_cmd_argument_t arg;
    wwd_result_t result;

    if ( direction == BUS_WRITE )
    {
        add_log_entry(direction, function, address, data_size, data);
    }

    arg.value = 0;
    arg.cmd53.function_number  = (unsigned int) ( function & BUS_FUNCTION_MASK );
    arg.cmd53.register_address = (unsigned int) ( address & 0x00001ffff );
    arg.cmd53.op_code = (unsigned int) 1;
    arg.cmd53.rw_flag = (unsigned int) ( ( direction == BUS_WRITE ) ? 1 : 0 );
    if ( mode == SDIO_BYTE_MODE )
    {
        wiced_assert( "wwd_bus_sdio_cmd53: data_size > 512 for byte mode", ( data_size <= (uint16_t) 512 ) );
        arg.cmd53.count = (unsigned int) ( data_size & 0x1FF );
    }
    else
    {
        arg.cmd53.count = (unsigned int) ( ( data_size / (uint16_t)SDIO_64B_BLOCK ) & 0x0000001ff );
        if ( (uint32_t) ( arg.cmd53.count * (uint16_t)SDIO_64B_BLOCK ) < data_size )
        {
            ++arg.cmd53.count;
        }
        arg.cmd53.block_mode = (unsigned int) 1;
    }

    result = host_platform_sdio_transfer( direction, SDIO_CMD_53, mode, SDIO_64B_BLOCK, arg.value, (uint32_t*) data, data_size, response_expected, response );

    if ( direction == BUS_READ )
    {
        add_log_entry(direction, function, address, data_size, data);
    }

    return result;
}

static wwd_result_t wwd_bus_sdio_download_firmware( void )
{
    uint8_t csr_val = 0;
    wwd_result_t result;
    uint32_t loop_count;

    VERIFY_RESULT( wwd_disable_device_core( ARM_CORE ) );
    VERIFY_RESULT( wwd_disable_device_core( SOCRAM_CORE ) );
    VERIFY_RESULT( wwd_reset_device_core( SOCRAM_CORE ) );

#if 0
    /* 43362 specific: Remap JTAG pins to UART output */
    uint32_t data = 0;
    VERIFY_RESULT( wwd_bus_write_backplane_value(0x18000650, 1, 1) );
    VERIFY_RESULT( wwd_bus_read_backplane_value(0x18000654, 4, (uint8_t*)&data) );
    data |= (1 << 24);
    VERIFY_RESULT( wwd_bus_write_backplane_value(0x18000654, 4, data) );
#endif


#ifdef MFG_TEST_ALTERNATE_WLAN_DOWNLOAD
    VERIFY_RESULT( external_write_wifi_firmware_and_nvram_image( ) );
#else
    VERIFY_RESULT( wwd_bus_write_wifi_firmware_image( ) );
    VERIFY_RESULT( wwd_bus_write_wifi_nvram_image( ) );
#endif /* ifdef MFG_TEST_ALTERNATE_WLAN_DOWNLOAD */

    /* Take the ARM core out of reset */
    VERIFY_RESULT( wwd_reset_device_core( ARM_CORE ) );

    result = wwd_device_core_is_up( ARM_CORE );
    if ( result != WWD_SUCCESS )
    {
        WPRINT_WWD_ERROR(("Could not bring ARM core up\n"));
        /*@-unreachable@*/ /* Reachable after hitting assert */
        return result;
        /*@+unreachable@*/
    }

    /* Wait until the High Throughput clock is available */
    loop_count = 0;
    while ( ( ( result = wwd_bus_sdio_read_register_value( BACKPLANE_FUNCTION, SDIO_CHIP_CLOCK_CSR, (uint8_t) 1, &csr_val ) ) == WWD_SUCCESS ) &&
            ( ( csr_val & SBSDIO_HT_AVAIL ) == 0 ) &&
            ( loop_count < (uint32_t) HT_AVAIL_TIMEOUT_MS ) )
    {
        (void) host_rtos_delay_milliseconds( (uint32_t) 1 ); /* Ignore return - nothing can be done if it fails */
        loop_count++;
    }
    if ( loop_count >= (uint32_t) HT_AVAIL_TIMEOUT_MS )
    {
        /* If your system times out here, it means that the WLAN firmware is not booting.
         * Check that your WLAN chip matches the 'wifi_image.c' being built - in GNU toolchain, $(CHIP)
         * makefile variable must be correct.
         */
         WPRINT_WWD_ERROR(("Timeout while waiting for high throughput clock\n"));
         /*@-unreachable@*/ /* Reachable after hitting assert */
         return WWD_TIMEOUT;
         /*@+unreachable@*/
    }
    if ( result != WWD_SUCCESS )
    {
        WPRINT_WWD_ERROR(("Error while waiting for high throughput clock\n"));
        /*@-unreachable@*/ /* Reachable after hitting assert */
        return result;
        /*@+unreachable@*/
    }

    /* Set up the interrupt mask and enable interrupts */
    VERIFY_RESULT( wwd_bus_write_backplane_value( SDIO_INT_HOST_MASK, (uint8_t) 4, I_HMB_SW_MASK ) );

    /* Enable F2 interrupts. This wasn't required for 4319 but is for the 43362 */
    VERIFY_RESULT( wwd_bus_write_backplane_value( SDIO_FUNCTION_INT_MASK, (uint8_t) 1, (uint32_t) 2) );

    /* Lower F2 Watermark to avoid DMA Hang in F2 when SD Clock is stopped. */
    VERIFY_RESULT( wwd_bus_write_register_value( BACKPLANE_FUNCTION, SDIO_FUNCTION2_WATERMARK, (uint8_t) 1, (uint32_t) SDIO_F2_WATERMARK ) );

    return WWD_SUCCESS;
}

/** Aborts a SDIO read of a packet from the 802.11 device
 *
 * This function is necessary because the only way to obtain the size of the next
 * available received packet is to read the first four bytes of the packet.
 * If the system reads these four bytes, and then fails to allocate the required
 * memory, then this function allows the system to abort the packet read cleanly,
 * and to optionally tell the 802.11 device to keep it allowing reception once
 * memory is available.
 *
 * In order to do this abort, the following actions are performed:
 * - Sets abort bit for Function 2 (WLAN Data) to request stopping transfer
 * - Sets Read Frame Termination bit to flush and reset fifos
 * - If packet is to be kept and resent by 802.11 device, a NAK  is sent
 * - Wait whilst the Fifo is emptied of the packet ( reading during this period would cause all zeros to be read )
 *
 * @param retry : WICED_TRUE if 802.11 device is to keep and resend packet
 *                WICED_FALSE if 802.11 device is to drop packet
 *
 * @return WWD_SUCCESS if successful, otherwise error code
 */
static wwd_result_t wwd_bus_sdio_abort_read( wiced_bool_t retry )
{
    /* Abort transfer on WLAN_FUNCTION */
    VERIFY_RESULT( wwd_bus_write_register_value( BUS_FUNCTION, SDIOD_CCCR_IOABORT, (uint8_t) 1, (uint32_t) WLAN_FUNCTION ) );

    /* Send frame terminate */
    VERIFY_RESULT( wwd_bus_write_register_value( BACKPLANE_FUNCTION, SDIO_FRAME_CONTROL, (uint8_t) 1, SFC_RF_TERM ) );

    /* If we want to retry message, send NAK */
    if ( retry == WICED_TRUE )
    {
        VERIFY_RESULT( wwd_bus_write_backplane_value( (uint32_t) SDIO_TO_SB_MAIL_BOX, (uint8_t) 1, SMB_NAK ) );
    }

    return WWD_SUCCESS;
}

static wwd_result_t wwd_bus_sdio_read_register_value( wwd_bus_function_t function, uint32_t address, uint8_t value_length, /*@out@*/ uint8_t* value )
{
    memset( value, 0, (size_t) value_length );
    return wwd_bus_sdio_transfer( BUS_READ, function, address, value_length, value, RESPONSE_NEEDED );
}

wwd_result_t wwd_bus_ensure_is_up( void )
{
    wwd_result_t result;
    uint8_t csr = 0;
    uint32_t attempts = (uint32_t) WLAN_BUS_UP_ATTEMPTS;

    /* Ensure HT clock is up */
    if (bus_is_up == WICED_TRUE)
    {
        return WWD_SUCCESS;
    }

    if ( WWD_SUCCESS != ( result = wwd_bus_write_register_value( BACKPLANE_FUNCTION, (uint32_t) SDIO_CHIP_CLOCK_CSR, (uint8_t) 1, (uint32_t) SBSDIO_HT_AVAIL_REQ | SBSDIO_FORCE_HT ) ) )
    {
        return result;
    }

    do
    {
        if ( WWD_SUCCESS != ( result = wwd_bus_sdio_read_register_value( BACKPLANE_FUNCTION, (uint32_t) SDIO_CHIP_CLOCK_CSR, (uint8_t) 1, &csr ) ) )
        {
            return result;
        }
        --attempts;
    }
    while ( ( ( csr & SBSDIO_HT_AVAIL ) == 0 ) &&
            ( attempts != 0 ) &&
            ( host_rtos_delay_milliseconds( (uint32_t) 1 ), 1==1 ) );

    if (attempts == 0)
    {
        return WWD_SDIO_BUS_UP_FAIL;
    }
    else
    {
        bus_is_up = WICED_TRUE;
        return WWD_SUCCESS;
    }
}

wwd_result_t wwd_bus_poke_wlan( void )
{
    /*TODO: change 1<<3 to a register hash define */
    return wwd_bus_write_backplane_value( SDIO_TO_SB_MAILBOX, (uint8_t) 4, (uint32_t)( 1 << 3 ) );
}

wwd_result_t wwd_bus_set_flow_control( uint8_t value )
{
    if ( value != 0 )
    {
        wwd_bus_flow_controlled = WICED_TRUE;
    }
    else
    {
        wwd_bus_flow_controlled = WICED_FALSE;
    }
    return WWD_SUCCESS;
}

wiced_bool_t wwd_bus_is_flow_controlled( void )
{
    return wwd_bus_flow_controlled;
}

wwd_result_t wwd_bus_allow_wlan_bus_to_sleep( void )
{
    /* Clear HT clock request */
    if (bus_is_up == WICED_TRUE)
    {
        bus_is_up = WICED_FALSE;
        return wwd_bus_write_register_value( BACKPLANE_FUNCTION, (uint32_t) SDIO_CHIP_CLOCK_CSR, (uint8_t) 1, 0 );
    }
    else
    {
        return WWD_SUCCESS;
    }
}

#ifndef WICED_DISABLE_MCU_POWERSAVE
static wwd_result_t wwd_bus_sdio_set_oob_interrupt( uint8_t gpio_pin_number )
{
    if ( gpio_pin_number == 1 )
    {
        /* Redirect to OOB interrupt to GPIO1 */
        VERIFY_RESULT( wwd_bus_write_register_value( BACKPLANE_FUNCTION, SDIO_GPIO_SELECT, (uint8_t)1, (uint32_t) 0xF ) );
        VERIFY_RESULT( wwd_bus_write_register_value( BACKPLANE_FUNCTION, SDIO_GPIO_OUTPUT, (uint8_t)1, (uint32_t) 0x0 ) );

        /* Enable GPIOx (bit x) */
        VERIFY_RESULT( wwd_bus_write_register_value( BACKPLANE_FUNCTION, SDIO_GPIO_ENABLE, (uint8_t)1, (uint32_t)0x2 ) );

        /* Set GPIOx (bit x) on Chipcommon GPIO Control register */
        VERIFY_RESULT( wwd_bus_write_register_value( BACKPLANE_FUNCTION, CHIPCOMMON_GPIO_CONTROL, (uint8_t)4, (uint32_t)0x2 ) );
    }

    return WWD_SUCCESS;
}
#endif
