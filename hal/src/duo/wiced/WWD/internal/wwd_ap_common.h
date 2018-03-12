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
 *  Provides prototypes / declarations for common APSTA functionality
 */
#ifndef _WWD_INTERNAL_AP_COMMON_H_
#define _WWD_INTERNAL_AP_COMMON_H_
#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/
#define AMPDU_RX_FACTOR_8K          0     /* max receive AMPDU length is 8kb */
#define AMPDU_RX_FACTOR_16K         1     /* max receive AMPDU length is 16kb */
#define AMPDU_RX_FACTOR_32K         2     /* max receive AMPDU length is 32kb */
#define AMPDU_RX_FACTOR_64K         3     /* max receive AMPDU length is 64kb */
#define AMPDU_RX_FACTOR_INVALID  0xff     /* invalid rx factor; ignore */
#define AMPDU_MPDU_AUTO             (-1)  /* Auto number of mpdu in ampdu */

#define htod32(i) ((uint32_t)(i))
#define htod16(i) ((uint16_t)(i))
#define dtoh32(i) ((uint32_t)(i))
#define dtoh16(i) ((uint16_t)(i))
#define CHECK_IOCTL_BUFFER( buff )  if ( buff == NULL ) { wiced_assert("Buffer alloc failed\n", 0); return WWD_BUFFER_ALLOC_FAIL; }
#define CHECK_IOCTL_BUFFER_WITH_SEMAPHORE( buff, sema )  if ( buff == NULL ) { wiced_assert("Buffer alloc failed\n", 0 == 1 ); (void) host_rtos_deinit_semaphore( sema ); return WWD_BUFFER_ALLOC_FAIL; }
#define CHECK_RETURN( expr )  { wwd_result_t check_res = (expr); if ( check_res != WWD_SUCCESS ) { wiced_assert("Command failed\n", 0 == 1 ); return check_res; } }
#define CHECK_RETURN_WITH_SEMAPHORE( expr, sema )  { wwd_result_t check_res = (expr); if ( check_res != WWD_SUCCESS ) { wiced_assert("Command failed\n", 0 == 1 ); (void) host_rtos_deinit_semaphore( sema ); return check_res; } }

/******************************************************
 *             Function prototypes
 ******************************************************/
extern wwd_result_t wwd_wifi_set_block_ack_window_size_common( wwd_interface_t interface, uint16_t ap_win_size, uint16_t sta_win_size );
extern wwd_result_t wwd_wifi_set_ampdu_parameters_common( wwd_interface_t interface, uint8_t ba_window_size, int8_t ampdu_mpdu, uint8_t rx_factor );

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* ifndef _WWD_INTERNAL_AP_COMMON_H_ */
