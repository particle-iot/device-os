#include "dct_hal.h"
#include "wiced_result.h"
#include "wiced_dct_common.h"
#include "waf_platform.h"
#include "module_info.h"
#include "service_debug.h"
#include "hal_irq_flag.h"

extern uint32_t Compute_CRC32(const uint8_t *pBuffer, uint32_t bufferSize);

// Compatibility crc32 function for WICED
uint32_t crc32(uint8_t* pdata, unsigned int nbytes, uint32_t crc) {
   uint32_t crcres = Compute_CRC32(pdata, nbytes);
   // We need to XOR computed CRC32 value with 0xffffffff again as it was already xored in Compute_CRC32
   // and apply passed crc value instead.
   crcres ^= 0xffffffff;
   crcres ^= crc;
   return crcres;
}

const void* dct_read_app_data(uint32_t offset) {
    return ((const void*)((uint32_t)wiced_dct_get_current_address(DCT_APP_SECTION) + offset));
}

int dct_write_app_data(const void* data, uint32_t offset, uint32_t size) {
    int res = wiced_dct_write(data, DCT_APP_SECTION, offset, size);

    return res;
}
