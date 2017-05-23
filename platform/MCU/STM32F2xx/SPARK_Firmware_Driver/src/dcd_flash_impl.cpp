
#include "dcd_flash_impl.h"
#include "atomic_flag_mutex.h"


/**
 * The DCD is called before constructors have executed (from HAL_Core_Config) so we need to manually construct
 * rather than rely upon global construction.
 */

uint32_t calculateCRC(const void* data, size_t len) {
	return Compute_CRC32(reinterpret_cast<const uint8_t*>(data), len);
}

UpdateDCD<InternalFlashStore, 16*1024, 0x8004000, 0x8008000, calculateCRC>& dcd()
{
	static UpdateDCD<InternalFlashStore, 16*1024, 0x8004000, 0x8008000, calculateCRC> dcd;
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
