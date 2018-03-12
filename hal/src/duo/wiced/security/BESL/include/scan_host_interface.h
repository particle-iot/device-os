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

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*scan_result_handler_t)(wl_escan_result_t* result, void* user_data);
typedef void (*scan_complete_hander_t)(void);

extern void besl_host_scan(scan_result_handler_t result_handler, void* user_data);

#ifdef __cplusplus
} /*extern "C" */
#endif

