#ifndef BOOTLOADER_DCT_H
#define BOOTLOADER_DCT_H

#include "platforms.h"

#if PLATFORM_ID == PLATFORM_PHOTON_PRODUCTION /* FIMXE: PLATFORM_P1 */
// Whether to load DCT functions dynamically or link them statically
#define LOAD_DCT_FUNCTIONS
#endif

#ifdef LOAD_DCT_FUNCTIONS
int load_dct_functions();
#endif

#endif // BOOTLOADER_DCT_H
