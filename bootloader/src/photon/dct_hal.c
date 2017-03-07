#include <stdint.h>

#ifndef CRC_INIT_VALUE
#define CRC_INIT_VALUE 0xffffffff
#endif

#ifndef CRC_TYPE
#define CRC_TYPE uint32_t
#endif

#include "../hal/src/photon/dct_hal.c"
