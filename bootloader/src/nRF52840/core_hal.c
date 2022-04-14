#include "core_hal.h"
#include "nrf_delay.h"

#define BACKUP_REGISTER_NUM        10
static int32_t backup_register[BACKUP_REGISTER_NUM] __attribute__((section(".backup_registers")));

int32_t HAL_Core_Backup_Register(uint32_t BKP_DR) {
    if ((BKP_DR == 0) || (BKP_DR > BACKUP_REGISTER_NUM)) {
        return -1;
    }

    return BKP_DR - 1;
}

void HAL_Core_Write_Backup_Register(uint32_t BKP_DR, uint32_t Data) {
    int32_t BKP_DR_Index = HAL_Core_Backup_Register(BKP_DR);
    if (BKP_DR_Index != -1) {
        backup_register[BKP_DR_Index] = Data;
    }
}

uint32_t HAL_Core_Read_Backup_Register(uint32_t BKP_DR) {
    int32_t BKP_DR_Index = HAL_Core_Backup_Register(BKP_DR);
    if (BKP_DR_Index != -1) {
        return backup_register[BKP_DR_Index];
    }
    return 0xFFFFFFFF;
}

void HAL_Delay_Microseconds(uint32_t uSec) {
    nrf_delay_us(uSec);
}

void HAL_Core_System_Reset_Ex(int reason, uint32_t data, void *reserved) {
    NVIC_SystemReset();
}

int HAL_Core_Enter_Panic_Mode(void* reserved) {
    __disable_irq();
    return 0;
}
