// pull in the sources from the HAL. It's a bit of a hack, but is simpler than trying to link the
// full hal library.
#define HAL_DEVICE_ID_NO_DCT
#include "../src/stm32f2xx/deviceid_hal.c"
#include "bytes2hexbuf.h"
