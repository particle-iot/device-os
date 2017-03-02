#include <stdint.h>
#ifndef CRC_INIT_VALUE
#define CRC_INIT_VALUE                              0xffffffff

extern uint32_t Compute_CRC32(const uint8_t *pBuffer, uint32_t bufferSize);

uint32_t crc32(uint8_t* pdata, unsigned int nbytes, uint32_t crc) {
   uint32_t crcres = Compute_CRC32(pdata, nbytes);
   crcres ^= 0xffffffff;
   crcres ^= crc;
   return crcres;
}

#define CRC_FUNCTION(address, size, previous_value) (uint32_t)crc32(address, size, previous_value)
typedef uint32_t    CRC_TYPE;
#endif
#include "../hal/src/photon/dct_hal.c"
