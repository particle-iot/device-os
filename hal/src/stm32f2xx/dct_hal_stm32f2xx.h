#ifndef DCT_HAL_STM32F2XX_H
#define DCT_HAL_STM32F2XX_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int(*dct_lock_func_t)(int);
typedef int(*dct_unlock_func_t)(int);

int dct_lock(int write);
int dct_unlock(int write);

// Overrides default DCT locking implementation
void dct_set_lock_impl(dct_lock_func_t lock, dct_unlock_func_t unlock);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // DCT_HAL_STM32F2XX_H
