// FIXME: Remove it when USB CDC is implemented.
#if PLATFORM_ID == 12 || PLATFORM_ID == 13 || PLATFORM_ID == 14
#define HAL_USB_EXCLUDE
#endif

#ifndef HAL_USB_EXCLUDE
#include "hal_dynalib_usb.h"
#endif
