#ifndef DCT_HAL_STM32F2XX_H
#define DCT_HAL_STM32F2XX_H

#ifdef __cplusplus
extern "C" {
#endif

int dct_lock(int write);
int dct_unlock(int write);

void dct_set_lock_enabled(int enabled);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // DCT_HAL_STM32F2XX_H
