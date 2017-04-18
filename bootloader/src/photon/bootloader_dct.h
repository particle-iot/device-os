#ifndef BOOTLOADER_DCT_H
#define BOOTLOADER_DCT_H

#include "platforms.h"

#if PLATFORM_ID == PLATFORM_P1
// Enable dynamic loading of the DCT functions
#define LOAD_DCT_FUNCTIONS
#endif

#ifdef LOAD_DCT_FUNCTIONS
void load_dct_functions();
#endif

#endif // BOOTLOADER_DCT_H
