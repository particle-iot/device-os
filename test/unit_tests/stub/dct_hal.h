#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int dct_read_app_data_copy(uint32_t offset, void* ptr, size_t size);
int dct_write_app_data(const void* data, uint32_t offset, uint32_t size);

#ifdef __cplusplus
} // extern "C"
#endif
