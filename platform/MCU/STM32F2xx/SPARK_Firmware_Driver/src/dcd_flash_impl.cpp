
#include "dcd_flash_impl.h"

/**
 * The DCD is called before constructors have executed (from HAL_Core_Config) so we need to manually construct
 * rather than rely upon global construction.
 */

UpdateDCD<InternalFlashStore, 16*1024, 0x8004000, 0x8008000>& dcd()
{
	static UpdateDCD<InternalFlashStore, 16*1024, 0x8004000, 0x8008000> dcd;
	return dcd;
}

const void* dct_read_app_data (uint32_t offset)
{
    return dcd().read(offset);
}

int dct_write_app_data(const void* data, uint32_t offset, uint32_t size)
{
    return dcd().write(offset, data, size);
}

void dcd_migrate_data()
{
    dcd().migrate();
}
