#ifndef _FLASH_HAL_H
#define _FLASH_HAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef void (*flash_op_callback_t)(void * p_buf);


int hal_flash_init(void);
int hal_flash_write(uint32_t addr, const uint8_t * data_buf, uint32_t data_size);
int hal_flash_read(uint32_t addr, uint8_t * data_buf, uint32_t data_size);
int hal_flash_erase_sector(uint32_t addr, uint32_t num_sectors);
int hal_flash_copy_sector(uint32_t src_addr, uint32_t dest_addr, uint32_t data_size);

#ifdef __cplusplus
} // extern "C"
#endif

#endif

