#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Acquire the global peripherals lock. The lock is recursive.
 */
void periph_lock(void);

/**
 * Release the global peripherals lock.
 */
void periph_unlock(void);

#ifdef __cplusplus
}
#endif
