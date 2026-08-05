#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    size_t size;
    const char* category;
    size_t max_size;
    int level;
} system_boot_log_config;

#ifdef __cplusplus
extern "C" {
#endif

void system_enable_boot_log(bool enabled, const system_boot_log_config* config, void* reserved);
void system_flush_boot_log(int level, const char* category, void* reserved);

#ifdef __cplusplus
} // extern "C"
#endif
