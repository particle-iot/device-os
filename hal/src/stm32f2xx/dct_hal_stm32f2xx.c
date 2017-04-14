#include "dct_hal_stm32f2xx.h"

#include "dct.h"

const void* HAL_DCT_Read_App_Data(uint32_t offset, void* reserved) {
    return dct_read_app_data(offset);
}

int HAL_DCT_Write_App_Data(const void* data, uint32_t offset, uint32_t size, void* reserved) {
    return dct_write_app_data(data, offset, size);
}
