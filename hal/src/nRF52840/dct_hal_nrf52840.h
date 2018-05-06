#ifndef DCT_HAL_NRF52840_H
#define DCT_HAL_NRF52840_H

#ifdef __cplusplus
extern "C" {
#endif

int dct_lock(int write);
int dct_unlock(int write);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // DCT_HAL_NRF52840_H
