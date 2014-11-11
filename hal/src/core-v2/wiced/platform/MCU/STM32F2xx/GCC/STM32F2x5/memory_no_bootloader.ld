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
    
    DCT1_FLASH (rx) : ORIGIN = 0x08004000, LENGTH = 16K
    DCT2_FLASH (rx) : ORIGIN = 0x08008000, LENGTH = 16K
    APP_FLASH (rx)  : ORIGIN = 0x08020000, LENGTH = 1024K-0x20000
    SRAM      (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

