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
    BTLDR_VECTORS    (rx)  : ORIGIN = 0x08000000, LENGTH = 512
    SHARED_API       (rx)  : ORIGIN = 0x08000200, LENGTH = 512
    BTLDR_CODE       (rx)  : ORIGIN = 0x08000400, LENGTH = 15K
    DCT1_FLASH       (rx)  : ORIGIN = 0x08004000, LENGTH = 16K
    DCT2_FLASH       (rx)  : ORIGIN = 0x08008000, LENGTH = 16K
    APP_CODE         (rx)  : ORIGIN = 0x0800C000, LENGTH = 0xF4000  /* 976K */
    SRAM             (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
    BTLDR_SRAM       (rwx) : ORIGIN = 0x2001E000, LENGTH = 8k       /* Boot loader stack at the end. */
}
