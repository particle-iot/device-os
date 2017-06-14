#include "dct_hal.h"

#include "wiced_framework.h"
#include "wiced_dct_common.h"

#include <string.h>
#include <stdint.h>

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

int dct_read_app_data_copy(uint32_t offset, void* ptr, size_t size) {
    void* p = NULL;
    int result = -1;
    if (ptr && wiced_dct_read_lock(&p, WICED_FALSE, DCT_APP_SECTION, offset, 0) == WICED_SUCCESS) {
        memcpy(ptr, p, size);
        result = wiced_dct_read_unlock(NULL, WICED_FALSE);
    }
    return result;
}

const void* dct_read_app_data_lock(uint32_t offset) {
    void* ptr = NULL;
    // wiced_dct_read_lock calls wiced_dct_lock internally
    wiced_dct_read_lock(&ptr, WICED_FALSE, DCT_APP_SECTION, offset, 0);
    return (const void*)ptr;
}

int dct_read_app_data_unlock(uint32_t offset) {
    // wiced_dct_read_unlock calls wiced_dct_unlock internally
    return wiced_dct_read_unlock(NULL, WICED_FALSE);
}

const void* dct_read_app_data(uint32_t offset) {
    wiced_dct_lock(0);
    const char* ptr = ((const char*)wiced_dct_get_current_address(DCT_APP_SECTION) + offset);
    wiced_dct_unlock(0);
    return ptr;
}

int dct_write_app_data(const void* data, uint32_t offset, uint32_t size) {
    return wiced_dct_write(data, DCT_APP_SECTION, offset, size);
}

wiced_result_t wiced_dct_lock(int write) {
    return (dct_lock(write) == 0) ? WICED_SUCCESS : WICED_ERROR;
}

wiced_result_t wiced_dct_unlock(int write) {
    return (dct_unlock(write) == 0) ? WICED_SUCCESS : WICED_ERROR;
}
