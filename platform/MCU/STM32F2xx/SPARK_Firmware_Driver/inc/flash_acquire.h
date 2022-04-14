#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Acquire a thread-safe non-recurisve lock to the flash memory.
 */
extern void __flash_acquire(void);

/**
 * Release the previously acquired lock to flash memory. This MUST
 * be called after calling __flash_acquire() at the moment the current flash operation is complete.
 */
extern void __flash_release(void);

#ifdef __cplusplus
}
#endif
