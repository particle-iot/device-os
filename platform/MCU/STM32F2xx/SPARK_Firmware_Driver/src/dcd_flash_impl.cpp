
#include "dcd_flash_impl.h"
#include "dct.h"
#include "atomic_flag_mutex.h"


/**
 * The DCD is called before constructors have executed (from HAL_Core_Config) so we need to manually construct
 * rather than rely upon global construction.
 */

UpdateDCD<InternalFlashStore, 16*1024, 0x8004000, 0x8008000>& dcd()
{
	static UpdateDCD<InternalFlashStore, 16*1024, 0x8004000, 0x8008000> dcd;
	return dcd;
}

using DCDLock = AtomicFlagMutex<os_result_t, os_thread_yield>;

static DCDLock dcdLock;

class AutoDCDLock : public std::lock_guard<DCDLock> {
public:
    AutoDCDLock() : std::lock_guard<DCDLock>(dcdLock) {}
};

const void* dct_read_app_data(uint32_t offset)
{
    AutoDCDLock lock;
    return dcd().read(offset);
}

int dct_read_app_data_copy(uint32_t offset, void* ptr, size_t size)
{
    AutoDCDLock lock;
    if (ptr)
    {
        const void* fptr = dcd().read(offset);
        memcpy(ptr, fptr, size);
        return 0;
    }
    return 1;
}

const void* dct_read_app_data_lock(uint32_t offset)
{
    dcdLock.lock();
    return dcd().read(offset);
}

int dct_read_app_data_unlock(uint32_t offset)
{
    dcdLock.unlock();
    return 0;
}

int dct_write_app_data(const void* data, uint32_t offset, uint32_t size)
{
    AutoDCDLock lock;
    return dcd().write(offset, data, size);
}

void dcd_migrate_data()
{
    AutoDCDLock lock;
    dcd().migrate();
}
