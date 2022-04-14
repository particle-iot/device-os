#include "core_hal.h"

void HAL_Core_System_Reset_Ex(int reason, uint32_t data, void *reserved) {
    NVIC_SystemReset();
}
