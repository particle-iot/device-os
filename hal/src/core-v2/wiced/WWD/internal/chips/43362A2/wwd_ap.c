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
 *  Provides an APSTA functionality specific to the 43362A2
 */

#include <string.h>
#include "wwd_rtos.h"
#include "wwd_events.h"
#include "wwd_assert.h"
#include "wwd_management.h"
#include "network/wwd_buffer_interface.h"
#include "internal/wwd_sdpcm.h"
#include "internal/wwd_internal.h"
#include "internal/wwd_ap.h"
#include "platform_toolchain.h"

/******************************************************
 * @cond       Constants
 ******************************************************/

#define WLC_EVENT_MSG_LINK      (0x01)
#define RATE_SETTING_11_MBPS    (11000000 / 500000)

/* HT/AMPDU specific define */
#define AMPDU_RX_FACTOR_8K  0   /* max rcv ampdu len (8kb) */
#define AMPDU_RX_FACTOR_16K 1   /* max rcv ampdu len (16kb) */
#define AMPDU_RX_FACTOR_32K 2   /* max rcv ampdu len (32kb) */
#define AMPDU_RX_FACTOR_64K 3   /* max rcv ampdu len (64kb) */

typedef enum
{
    BSS_AP   = 3,
    BSS_STA  = 2,
    BSS_UP   = 1,
    BSS_DOWN = 0
} bss_arg_option_t;

/** @endcond */

#define htod32(i) ((uint32_t)(i))
#define htod16(i) ((uint16_t)(i))
#define dtoh32(i) ((uint32_t)(i))
#define dtoh16(i) ((uint16_t)(i))
#define CHECK_IOCTL_BUFFER( buff )  if ( buff == NULL ) { wiced_assert("Buffer alloc failed\n", 0); return WWD_BUFFER_ALLOC_FAIL; }
#define CHECK_IOCTL_BUFFER_WITH_SEMAPHORE( buff, sema )  if ( buff == NULL ) { wiced_assert("Buffer alloc failed\n", 0 == 1 ); (void) host_rtos_deinit_semaphore( sema ); return WWD_BUFFER_ALLOC_FAIL; }
#define CHECK_RETURN( expr )  { wwd_result_t check_res = (expr); if ( check_res != WWD_SUCCESS ) { wiced_assert("Command failed\n", 0 == 1 ); return check_res; } }
#define CHECK_RETURN_WITH_SEMAPHORE( expr, sema )  { wwd_result_t check_res = (expr); if ( check_res != WWD_SUCCESS ) { wiced_assert("Command failed\n", 0 == 1 ); (void) host_rtos_deinit_semaphore( sema ); return check_res; } }

/******************************************************
 *             Local Structures
 ******************************************************/


/******************************************************
 *             Static Variables
 ******************************************************/

wiced_bool_t                 wwd_wifi_ap_is_up = WICED_FALSE;
static host_semaphore_type_t wwd_wifi_sleep_flag;
static const wwd_event_num_t apsta_events[] = { WLC_E_IF, WLC_E_LINK, WLC_E_NONE };

/******************************************************
 *             Static Function prototypes
 ******************************************************/

static void*        wwd_handle_apsta_event ( const wwd_event_header_t* event_header, const uint8_t* event_data, /*@returned@*/ void* handler_user_data );
static wwd_result_t internal_ap_init       ( wiced_ssid_t* ssid, wiced_security_t auth_type, const uint8_t* security_key, uint8_t key_length, uint8_t channel );
wwd_result_t        wwd_wifi_ap_init       ( wiced_ssid_t* ssid, wiced_security_t auth_type, const uint8_t* security_key, uint8_t key_length, uint8_t channel );
wwd_result_t        wwd_wifi_ap_up         ( void );

/******************************************************
 *             Function definitions
 ******************************************************/

static void* wwd_handle_apsta_event( const wwd_event_header_t* event_header, const uint8_t* event_data, /*@returned@*/ void* handler_user_data )
{
    UNUSED_PARAMETER( event_header );
    UNUSED_PARAMETER( event_data );
    UNUSED_PARAMETER( handler_user_data );

    if ( (wwd_interface_t) event_header->interface != WWD_AP_INTERFACE)
    {
        return handler_user_data;
    }

    if ( ( ( event_header->event_type == (wwd_event_num_t) WLC_E_LINK ) &&
           ( event_header->interface == WWD_AP_INTERFACE ) ) ||
           ( event_header->event_type == WLC_E_IF ) )
    {
        wwd_result_t result;
        result = host_rtos_set_semaphore( &wwd_wifi_sleep_flag, WICED_FALSE );
        wiced_assert( "failed to post ap link semaphore", result == WWD_SUCCESS );
        REFERENCE_DEBUG_ONLY_VARIABLE( result );
    }
    return handler_user_data;
}

static wwd_result_t internal_ap_init( wiced_ssid_t* ssid, wiced_security_t auth_type, const uint8_t* security_key, uint8_t key_length, uint8_t channel )
{
    wiced_bool_t   wait_for_interface = WICED_FALSE;
    wwd_result_t   result;
    wiced_buffer_t response;
    wiced_buffer_t buffer;
    uint32_t*      data;

    if ( auth_type == WICED_SECURITY_WEP_PSK )
    {
        return WWD_WEP_NOT_ALLOWED;
    }
    if ( ( ( auth_type == WICED_SECURITY_WPA_TKIP_PSK ) || ( auth_type == WICED_SECURITY_WPA2_AES_PSK ) || ( auth_type == WICED_SECURITY_WPA2_MIXED_PSK ) ) &&
         ( ( key_length < (uint8_t) 8 ) || ( key_length > (uint8_t) 64 ) ) )
    {
        return WWD_WPA_KEYLEN_BAD;
    }

    if ( wwd_wifi_set_block_ack_window_size( WWD_AP_INTERFACE ) != WWD_SUCCESS )
    {
        return WWD_SET_BLOCK_ACK_WINDOW_FAIL;
    }

    /* Query bss state (does it exist? if so is it UP?) */
    data = (uint32_t*) wwd_sdpcm_get_iovar_buffer( &buffer, (uint16_t) 4, IOVAR_STR_BSS );
    CHECK_IOCTL_BUFFER( data );
    *data = (uint32_t) CHIP_AP_INTERFACE;
    if ( wwd_sdpcm_send_iovar( SDPCM_GET, buffer, &response, WWD_STA_INTERFACE ) != WWD_SUCCESS )
    {
        /* Note: We don't need to release the response packet since the iovar failed */
        wait_for_interface = WICED_TRUE;
    }
    else
    {
        /* Check if the BSS is already UP, if so return */
        uint32_t* data2 = (uint32_t*) host_buffer_get_current_piece_data_pointer( response );
        if ( *data2 == (uint32_t) BSS_UP )
        {
            host_buffer_release( response, WWD_NETWORK_RX );
            wwd_wifi_ap_is_up = WICED_TRUE;
            return WWD_SUCCESS;
        }
        else
        {
            host_buffer_release( response, WWD_NETWORK_RX );
        }
    }

    CHECK_RETURN( host_rtos_init_semaphore( &wwd_wifi_sleep_flag ) );

    /* Register for interested events */
    CHECK_RETURN_WITH_SEMAPHORE( wwd_management_set_event_handler( apsta_events, wwd_handle_apsta_event, NULL, WWD_AP_INTERFACE ), &wwd_wifi_sleep_flag );

    /* Set the SSID */
    data = (uint32_t*) wwd_sdpcm_get_iovar_buffer( &buffer, (uint16_t) 40, IOVAR_STR_BSSCFG_SSID );
    CHECK_IOCTL_BUFFER_WITH_SEMAPHORE( data, &wwd_wifi_sleep_flag );
    data[0] = (uint32_t) CHIP_AP_INTERFACE; /* Set the bsscfg index */
    data[1] = ssid->length; /* Set the ssid length */
    memcpy( &data[2], (uint8_t*) ssid->value, ssid->length );
    CHECK_RETURN_WITH_SEMAPHORE( wwd_sdpcm_send_iovar( SDPCM_SET, buffer, 0, WWD_STA_INTERFACE ), &wwd_wifi_sleep_flag );

    /* Check if we need to wait for interface to be created */
    if ( wait_for_interface == WICED_TRUE )
    {
        CHECK_RETURN_WITH_SEMAPHORE( host_rtos_get_semaphore( &wwd_wifi_sleep_flag, (uint32_t) 10000, WICED_FALSE ), &wwd_wifi_sleep_flag );
    }

    /* Set the channel */
    data = (uint32_t*) wwd_sdpcm_get_ioctl_buffer( &buffer, (uint16_t) 4 );
    CHECK_IOCTL_BUFFER_WITH_SEMAPHORE( data, &wwd_wifi_sleep_flag );
    *data = channel;
    CHECK_RETURN_WITH_SEMAPHORE( wwd_sdpcm_send_ioctl( SDPCM_SET, WLC_SET_CHANNEL, buffer, 0, WWD_AP_INTERFACE ), &wwd_wifi_sleep_flag );

    data = (uint32_t*) wwd_sdpcm_get_iovar_buffer( &buffer, (uint16_t) 8, IOVAR_STR_BSSCFG_WSEC );
    CHECK_IOCTL_BUFFER_WITH_SEMAPHORE( data, &wwd_wifi_sleep_flag );
    data[0] = (uint32_t) CHIP_AP_INTERFACE;
    if ((auth_type & WPS_ENABLED) != 0)
    {
        data[1] = (uint32_t) ( ( auth_type & ( ~WPS_ENABLED ) ) | SES_OW_ENABLED );
    }
    else
    {
        data[1] = (uint32_t) auth_type;
    }
    CHECK_RETURN_WITH_SEMAPHORE( wwd_sdpcm_send_iovar( SDPCM_SET, buffer, 0, WWD_STA_INTERFACE ), &wwd_wifi_sleep_flag );

    if ( auth_type != WICED_SECURITY_OPEN )
    {
        wsec_pmk_t* psk;

        /* Set the wpa auth */
        data = (uint32_t*) wwd_sdpcm_get_iovar_buffer( &buffer, (uint16_t) 8, IOVAR_STR_BSSCFG_WPA_AUTH );
        CHECK_IOCTL_BUFFER_WITH_SEMAPHORE( data, &wwd_wifi_sleep_flag );
        data[0] = (uint32_t) CHIP_AP_INTERFACE;
        data[1] = (uint32_t) (auth_type == WICED_SECURITY_WPA_TKIP_PSK) ? ( WPA_AUTH_PSK ) : ( WPA2_AUTH_PSK | WPA_AUTH_PSK );
        CHECK_RETURN_WITH_SEMAPHORE( wwd_sdpcm_send_iovar( SDPCM_SET, buffer, 0, WWD_STA_INTERFACE ), &wwd_wifi_sleep_flag );

        /* Set the passphrase */
        psk = (wsec_pmk_t*) wwd_sdpcm_get_ioctl_buffer( &buffer, sizeof(wsec_pmk_t) );
        CHECK_IOCTL_BUFFER_WITH_SEMAPHORE( psk, &wwd_wifi_sleep_flag );
        memcpy( psk->key, security_key, key_length );
        psk->key_len = key_length;
        psk->flags   = (uint16_t) WSEC_PASSPHRASE;
        host_rtos_delay_milliseconds( 1 ); /* Delay required to allow radio firmware to be ready to receive PMK and avoid intermittent failure */
        CHECK_RETURN_WITH_SEMAPHORE( wwd_sdpcm_send_ioctl( SDPCM_SET, WLC_SET_WSEC_PMK, buffer, 0, WWD_AP_INTERFACE ), &wwd_wifi_sleep_flag );
    }

    /* Set the GMode */
    data = (uint32_t*) wwd_sdpcm_get_ioctl_buffer( &buffer, (uint16_t) 4 );
    CHECK_IOCTL_BUFFER_WITH_SEMAPHORE( data, &wwd_wifi_sleep_flag );
    *data = (uint32_t) GMODE_AUTO;

    result =  wwd_sdpcm_send_ioctl( SDPCM_SET, WLC_SET_GMODE, buffer, 0, WWD_AP_INTERFACE );
    if ( ( result != WWD_SUCCESS ) && ( result != WWD_WLAN_ASSOCIATED ) )
    {
        wiced_assert("start_ap: Failed to set GMode\n", 0 == 1 );

        (void) host_rtos_deinit_semaphore( &wwd_wifi_sleep_flag );
        return result;
    }

    /* Set the multicast transmission rate to 11 Mbps rather than the default 1 Mbps */
    data = (uint32_t*) wwd_sdpcm_get_iovar_buffer( &buffer, (uint16_t) 4, IOVAR_STR_2G_MULTICAST_RATE );
    CHECK_IOCTL_BUFFER( data );
    *data = (uint32_t) RATE_SETTING_11_MBPS;
    result = wwd_sdpcm_send_iovar( SDPCM_SET, buffer, NULL, WWD_AP_INTERFACE );
    wiced_assert("start_ap: Failed to set multicast transmission rate\r\n", result == WWD_SUCCESS );

    /* Set DTIM period */
    data = (uint32_t*) wwd_sdpcm_get_ioctl_buffer( &buffer, (uint16_t) 4 );
    CHECK_IOCTL_BUFFER_WITH_SEMAPHORE( data, &wwd_wifi_sleep_flag );
    *data = (uint32_t) WICED_DEFAULT_SOFT_AP_DTIM_PERIOD;
    CHECK_RETURN_WITH_SEMAPHORE( wwd_sdpcm_send_ioctl( SDPCM_SET, WLC_SET_DTIMPRD, buffer, 0, WWD_AP_INTERFACE ), &wwd_wifi_sleep_flag );

#ifdef WICED_DISABLE_SSID_BROADCAST
    /* Make the AP "hidden" */
    data = (uint32_t*) wwd_sdpcm_get_iovar_buffer( &buffer, (uint16_t) 4, IOVAR_STR_CLOSEDNET );
    CHECK_IOCTL_BUFFER_WITH_SEMAPHORE( data, &wwd_wifi_sleep_flag );
    data[0] = (uint32_t) 1;
    CHECK_RETURN_WITH_SEMAPHORE( wwd_sdpcm_send_iovar( SDPCM_SET, buffer, 0, WWD_AP_INTERFACE ), &wwd_wifi_sleep_flag );
#endif

    return WWD_SUCCESS;
}

wwd_result_t wwd_wifi_ap_init( wiced_ssid_t* ssid, wiced_security_t auth_type, const uint8_t* security_key, uint8_t key_length, uint8_t channel )
{
    wwd_result_t result;

    /* Keep WLAN awake while setting up softAP */
    WWD_WLAN_KEEP_AWAKE( );

    result = internal_ap_init( ssid, auth_type, security_key, key_length, channel );

    WWD_WLAN_LET_SLEEP( );

    return result;
}

wwd_result_t wwd_wifi_ap_up( void )
{
    wiced_buffer_t buffer;
    uint32_t*      data;

    data = (uint32_t*) wwd_sdpcm_get_iovar_buffer( &buffer, (uint16_t) 8, IOVAR_STR_BSS );
    CHECK_IOCTL_BUFFER_WITH_SEMAPHORE( data, &wwd_wifi_sleep_flag );
    data[0] = (uint32_t) CHIP_AP_INTERFACE;
    data[1] = (uint32_t) BSS_UP;
    CHECK_RETURN_WITH_SEMAPHORE( wwd_sdpcm_send_iovar( SDPCM_SET, buffer, 0, WWD_STA_INTERFACE ), &wwd_wifi_sleep_flag );

    /* Wait until AP is brought up */
    CHECK_RETURN_WITH_SEMAPHORE( host_rtos_get_semaphore( &wwd_wifi_sleep_flag, (uint32_t) 10000, WICED_FALSE ), &wwd_wifi_sleep_flag );

    wwd_wifi_ap_is_up = WICED_TRUE;
    return WWD_SUCCESS;
}

/** Starts an infrastructure WiFi network
 * @param ssid      : A null terminated string containing the SSID name of the network to join
 * @param auth_type  : Authentication type:
 *                    - WICED_SECURITY_OPEN - Open Security
 *                    - WICED_SECURITY_WPA_TKIP_PSK   - WPA Security
 *                    - WICED_SECURITY_WPA2_AES_PSK   - WPA2 Security using AES cipher
 *                    - WICED_SECURITY_WPA2_MIXED_PSK - WPA2 Security using AES and/or TKIP ciphers
 *                    - WEP security is currently unimplemented due to lack of security
 * @param security_key : A byte array containing the cleartext security key for the network
 * @param key_length   : The length of the security_key in bytes.
 * @param channel     : 802.11 Channel number
 *
 * @return    WWD_SUCCESS : if successfully creates an AP
 *            Error code  : if an error occurred
 */
wwd_result_t wwd_wifi_start_ap( wiced_ssid_t* ssid, wiced_security_t auth_type, const uint8_t* security_key, uint8_t key_length, uint8_t channel )
{
    CHECK_RETURN( wwd_wifi_ap_init( ssid, auth_type, security_key, key_length, channel ) );

    CHECK_RETURN( wwd_wifi_ap_up() );

    return WWD_SUCCESS;
}

wwd_result_t wwd_wifi_stop_ap( void )
{
    uint32_t* data;
    wiced_buffer_t buffer;
    wiced_buffer_t response;
    wwd_result_t result;
    wwd_result_t result2;

    /* Query bss state (does it exist? if so is it UP?) */
    data = (uint32_t*) wwd_sdpcm_get_iovar_buffer( &buffer, (uint16_t) 4, IOVAR_STR_BSS );
    CHECK_IOCTL_BUFFER( data );
    *data = (uint32_t) CHIP_AP_INTERFACE;
    result = wwd_sdpcm_send_iovar( SDPCM_GET, buffer, &response, WWD_STA_INTERFACE );
    if ( result == WWD_WLAN_NOTFOUND )
    {
        /* AP interface does not exist - i.e. it is down */
        wwd_wifi_ap_is_up = WICED_FALSE;
        return WWD_SUCCESS;
    }

    CHECK_RETURN( result );

    data = (uint32_t*) host_buffer_get_current_piece_data_pointer( response );
    if ( data[0] != (uint32_t) BSS_UP )
    {
        /* AP interface indicates it is not up - i.e. it is down */
        host_buffer_release( response, WWD_NETWORK_RX );
        wwd_wifi_ap_is_up = WICED_FALSE;
        return WWD_SUCCESS;
    }

    host_buffer_release( response, WWD_NETWORK_RX );

    /* set BSS down */
    data = (uint32_t*) wwd_sdpcm_get_iovar_buffer( &buffer, (uint16_t) 8, IOVAR_STR_BSS );
    CHECK_IOCTL_BUFFER( data );
    data[0] = (uint32_t) CHIP_AP_INTERFACE;
    data[1] = (uint32_t) BSS_DOWN;
    CHECK_RETURN( wwd_sdpcm_send_iovar( SDPCM_SET, buffer, 0, WWD_AP_INTERFACE ) );

    /* Wait until AP is brought down */
    result = host_rtos_get_semaphore( &wwd_wifi_sleep_flag, (uint32_t) 10000, WICED_FALSE );
    result2 = host_rtos_deinit_semaphore( &wwd_wifi_sleep_flag );
    if ( result != WWD_SUCCESS )
    {
        return result;
    }
    if ( result2 != WWD_SUCCESS )
    {
        return result2;
    }

    CHECK_RETURN( wwd_management_set_event_handler( apsta_events, NULL, NULL, WWD_AP_INTERFACE ) );

    wwd_wifi_ap_is_up = WICED_FALSE;
    return WWD_SUCCESS;

}

wiced_bool_t wwd_wifi_is_packet_from_ap( uint8_t flags2 )
{
#define BDC_FLAG2_IF_MASK   0x0f
    if ((flags2 & BDC_FLAG2_IF_MASK) != 0)
    {
        return WICED_TRUE;
    }
    else
    {
        return WICED_FALSE;
    }
}

/** Sets the chip specific AMPDU parameters for AP and STA
 *  For SDK 3.0, and beyond, each chip will need it's own function for setting AMPDU parameters.
 */

wwd_result_t wwd_wifi_set_ampdu_parameters( void )
{
    wiced_buffer_t buffer;
    wwd_result_t retval;

    /* Set AMPDU Block ACK window size */
    uint32_t* data = (uint32_t*) wwd_sdpcm_get_iovar_buffer( &buffer, (uint16_t) 4, IOVAR_STR_AMPDU_BA_WINDOW_SIZE );
    CHECK_IOCTL_BUFFER( data );
    *data = (uint32_t) 8;
    retval = wwd_sdpcm_send_iovar( SDPCM_SET, buffer, NULL, WWD_STA_INTERFACE );

    wiced_assert("set_ampdu_parameters: Failed to set block ack window size\r\n", retval == WWD_SUCCESS );

    /* Set number of MPDUs available for AMPDU */
    data = (uint32_t*) wwd_sdpcm_get_iovar_buffer( &buffer, (uint16_t) 4, IOVAR_STR_AMPDU_MPDU );
    CHECK_IOCTL_BUFFER( data );
    *data = (uint32_t) 4;
    retval = wwd_sdpcm_send_iovar( SDPCM_SET, buffer, NULL, WWD_STA_INTERFACE );

    wiced_assert("set_ampdu_parameters: Failed to set number of MPDUs\r\n", retval == WWD_SUCCESS );

    /* Set size of advertised receive AMPDU */
    data = (uint32_t*) wwd_sdpcm_get_iovar_buffer( &buffer, (uint16_t) 4, IOVAR_STR_AMPDU_RX_FACTOR );
    CHECK_IOCTL_BUFFER( data );
    *data = (uint32_t) AMPDU_RX_FACTOR_8K;
    retval = wwd_sdpcm_send_iovar( SDPCM_SET, buffer, NULL, WWD_STA_INTERFACE );

    wiced_assert("set_ampdu_parameters: Failed to set advertised receive AMPDU size\r\n", retval == WWD_SUCCESS );

    return retval;
}

/** Sets the chip specific AMPDU parameters for AP and STA
 *  For SDK 3.0, and beyond, each chip will need it's own function for setting AMPDU parameters.
 */

wwd_result_t wwd_wifi_set_block_ack_window_size( wwd_interface_t interface )
{
    wiced_buffer_t buffer;
    wwd_result_t retval;
    uint32_t block_ack_window_size = 2;
    uint32_t* data = NULL;

    /* If the AP interface is already up then don't change the Block Ack window size */
    if ( wwd_wifi_is_ready_to_transceive( WWD_AP_INTERFACE ) == WWD_SUCCESS )
    {
        return WWD_SUCCESS;
    }

    /* AP can handle BA window size of 1 but STA can handle BA window size of 8 */
    if ( interface == WWD_STA_INTERFACE )
    {
        block_ack_window_size = 8;
    }

    /* Set AMPDU Block ACK window size */
    data = (uint32_t*) wwd_sdpcm_get_iovar_buffer( &buffer, (uint16_t) 4, IOVAR_STR_AMPDU_BA_WINDOW_SIZE );
    CHECK_IOCTL_BUFFER( data );
    *data = block_ack_window_size;
    retval = wwd_sdpcm_send_iovar( SDPCM_SET, buffer, NULL, WWD_STA_INTERFACE );

    wiced_assert("set_block_ack_window_size: Failed to set block ack window size\r\n", retval == WWD_SUCCESS );

    return retval;
}
