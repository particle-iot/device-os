/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/*
 * Defines WICED-related configuration.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*  WICED Resources uses a filesystem */
#define USES_RESOURCE_FILESYSTEM

/* Location on SPI Flash where filesystem starts */
#define FILESYSTEM_BASE_ADDR (0x00010000)

/* The main app is stored in external serial flash */
#define BOOTLOADER_LOAD_MAIN_APP_FROM_EXTERNAL_LOCATION


/*  DCT is stored in external flash */
#define EXTERNAL_DCT

#ifdef __cplusplus
} /* extern "C" */
#endif

