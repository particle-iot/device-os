#ifndef DCT_HAL_STM32F2XX_H
#define DCT_HAL_STM32F2XX_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

const void* HAL_DCT_Read_App_Data(uint32_t offset, void* reserved);
int HAL_DCT_Write_App_Data(const void* data, uint32_t offset, uint32_t size, void* reserved);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // DCT_HAL_STM32F2XX_H
