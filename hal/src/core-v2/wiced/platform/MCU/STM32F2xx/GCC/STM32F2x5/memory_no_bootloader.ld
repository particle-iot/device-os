/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

MEMORY
{
    /* No Bootloader or DCT */
    /* Offset for bootloader */
    APP_FLASH (rx)  : ORIGIN = 0x08020000, LENGTH = 1024K-0x20000
    SRAM      (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

