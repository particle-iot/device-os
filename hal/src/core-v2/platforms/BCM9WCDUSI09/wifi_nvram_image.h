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
 *  NVRAM variables which define BCM43362 Parameters for the
 *  USI module used on the BCM943362WCD4_3 board
 *
 */

#ifndef INCLUDED_NVRAM_IMAGE_H_
#define INCLUDED_NVRAM_IMAGE_H_

#include <string.h>
#include <stdint.h>
#include "../generated_mac_address.txt"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Character array of NVRAM image
 */

static const char wifi_nvram_image[] =
        "cbuckout=1500"                                                      "\x00"
        "sromrev=3"                                                          "\x00"
        "boardtype=0x05a0"                                                   "\x00"
        "boardrev=0x1203"                                                    "\x00"
        "manfid=0x2d0"                                                       "\x00"
        "prodid=0x492"                                                       "\x00"
        "vendid=0x14e4"                                                      "\x00"
        "devid=0x4343"                                                       "\x00"
        "boardflags=0x200"                                                   "\x00"
        "nocrc=1"                                                            "\x00"
        "xtalfreq=26000"                                                     "\x00"
        "boardnum=777"                                                       "\x00"
        NVRAM_GENERATED_MAC_ADDRESS                                          "\x00"
        "aa2g=3"                                                             "\x00"
        "ag0=0"                                                              "\x00"
        "ccode=ww"                                                           "\x00"
        "pa0b0= 0x13F9"                                                      "\x00"
        "pa0b1= 0xFD93"                                                      "\x00"
        "pa0b2= 0xFF4D"                                                      "\x00"
        "rssismf2g=0xa"                                                      "\x00"
        "rssismc2g=0x3"                                                      "\x00"
        "rssisav2g=0x7"                                                      "\x00"
        "maxp2ga0=0x46"                                                      "\x00"
        "cck2gpo=0x0"                                                        "\x00"
        "ofdm2gpo=0x22222222"                                                "\x00"
        "mcs2gpo0=0x3333"                                                    "\x00"
        "mcs2gpo1=0x6333"                                                    "\x00"
        "wl0id=0x431b"                                                       "\x00"
        "cckdigfilttype=22"                                                  "\x00"
        "cckPwrOffset=5"                                                     "\x00"
        "ofdmanalogfiltbw2g=3"                                               "\x00"
        "rfreg033=0x19"                                                      "\x00"
        "rfreg033_cck=0x1f"                                                  "\x00"
        "noise_cal_enable_2g=0"                                              "\x00"
        "pacalidx2g=10"                                                      "\x00"
        "swctrlmap_2g=0x0c050c05,0x0a030a03,0x0a030a03,0x0,0x1ff"            "\x00"
        "triso2g=1"                                                          "\x00"
        "RAW1=4a 0b ff ff 20 04 d0 02 62 a9"                                 "\x00"
        "otpimagesize=76"                                                    "\x00"
        "edonthd=-70"                                                        "\x00"
        "edoffthd=-76"                                                       "\x00"
        "\x00\x00";


#ifdef __cplusplus
} /* extern "C" */
#endif

#else /* ifndef INCLUDED_NVRAM_IMAGE_H_ */

#error Wi-Fi NVRAM image included twice

#endif /* ifndef INCLUDED_NVRAM_IMAGE_H_ */
