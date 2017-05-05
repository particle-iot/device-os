
#include "dcd_flash_impl.h"
#include "dct.h"
#include "concurrent_hal.h"

/**
 * The DCD is called before constructors have executed (from HAL_Core_Config) so we need to manually construct
 * rather than rely upon global construction.
 */

UpdateDCD<InternalFlashStore, 16*1024, 0x8004000, 0x8008000>& dcd()
{
	static UpdateDCD<InternalFlashStore, 16*1024, 0x8004000, 0x8008000> dcd;
	return dcd;
}

const void* dct_read_app_data(uint32_t offset)
{
    dct_lock(0, NULL);
    const void* ptr = dcd().read(offset);
    dct_unlock(0, NULL);
    return ptr;
}

int dct_read_app_data_copy(uint32_t offset, void* ptr, size_t size)
{
    int res = 1;
    dct_lock(0, NULL);
    if (ptr)
    {
        const void* fptr = dcd().read(offset);
        memcpy(ptr, fptr, size);
        res = 0;
    }
    dct_unlock(0, NULL);
    return res;
}

const void* dct_read_app_data_lock(uint32_t offset)
{
    dct_lock(0, NULL);
    return dcd().read(offset);
}

int dct_read_app_data_unlock(uint32_t offset)
{
    return dct_unlock(0, NULL);
}

int dct_write_app_data(const void* data, uint32_t offset, uint32_t size)
{
    dct_lock(1, NULL);
    int res = dcd().write(offset, data, size);
    dct_unlock(1, NULL);
    return res;
}

void dcd_migrate_data()
{
    dct_lock(1, NULL);
    dcd().migrate();
    dct_unlock(1, NULL);
}
