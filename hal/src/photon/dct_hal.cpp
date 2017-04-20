#include "dct_hal.h"
#include "wiced_result.h"
#include "wiced_dct_common.h"
#include "waf_platform.h"
#include "wiced_framework.h"
#include "module_info.h"
#include "service_debug.h"
#include "hal_irq_flag.h"
#include "atomic_flag_mutex.h"

using DCDLock = AtomicFlagMutex<os_result_t, os_thread_yield>;

static DCDLock dcdLock;

extern uint32_t Compute_CRC32(const uint8_t *pBuffer, uint32_t bufferSize);

// Compatibility crc32 function for WICED
extern "C" uint32_t crc32(uint8_t* pdata, unsigned int nbytes, uint32_t crc) {
   uint32_t crcres = Compute_CRC32(pdata, nbytes);
   // We need to XOR computed CRC32 value with 0xffffffff again as it was already xored in Compute_CRC32
   // and apply passed crc value instead.
   crcres ^= 0xffffffff;
   crcres ^= crc;
   return crcres;
}

int dct_read_app_data_copy(uint32_t offset, void* ptr, size_t size)
{
    void* p = NULL;
    int result = 0;
    if (ptr && wiced_dct_read_lock(&p, WICED_FALSE, DCT_APP_SECTION, offset, 0) == WICED_SUCCESS)
    {
        memcpy(ptr, p, size);
    }
    else
    {
        result = 1;
    }
    wiced_dct_read_unlock(NULL, WICED_FALSE);

    return result;
}

const void* dct_read_app_data_lock(uint32_t offset)
{
    void* ptr = NULL;
    wiced_dct_read_lock(&ptr, WICED_FALSE, DCT_APP_SECTION, offset, 0);
    return (const void*)ptr;
}

int dct_read_app_data_unlock(uint32_t offset)
{
    return wiced_dct_read_unlock(NULL, WICED_FALSE);
}

const void* dct_read_app_data(uint32_t offset) {
    return ((const void*)((uint32_t)wiced_dct_get_current_address(DCT_APP_SECTION) + offset));
}

int dct_write_app_data(const void* data, uint32_t offset, uint32_t size) {
    int res = wiced_dct_write(data, DCT_APP_SECTION, offset, size);

    return res;
}

wiced_result_t wiced_dct_lock()
{
    dcdLock.lock();
    return WICED_SUCCESS;
}

wiced_result_t wiced_dct_unlock()
{
    dcdLock.unlock();
    return WICED_SUCCESS;
}
