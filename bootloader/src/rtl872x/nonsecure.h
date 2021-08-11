#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void nonsecure_init();
void nonsecure_jump_to_system(uint32_t addr);

#ifdef __cplusplus
}
#endif
