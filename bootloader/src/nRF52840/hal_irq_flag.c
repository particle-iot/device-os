#include "nrf52840.h"
#include "nrf_nvic.h"

int HAL_disable_irq()
{
    uint8_t st = 0;
    sd_nvic_critical_region_enter(&st);
    return st;
}

void HAL_enable_irq(int is) {
    uint8_t st = (uint8_t)is;
    sd_nvic_critical_region_exit(st);
}

void app_util_critical_region_enter(uint8_t* nested) {
    (void)sd_nvic_critical_region_enter(nested);
}

void app_util_critical_region_exit(uint8_t nested) {
    (void)sd_nvic_critical_region_exit(nested);
}
