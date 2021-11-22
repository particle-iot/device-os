/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "core_hal.h"
#include "hw_ticks.h"
#include <limits.h>
#include "rtl8721d.h"
#include "km0_km4_ipc.h"

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
    volatile uint32_t DWT_START = DWT->CYCCNT;

    if (uSec > (UINT_MAX / SYSTEM_US_TICKS)) {
        uSec = (UINT_MAX / SYSTEM_US_TICKS);
    }

    volatile uint32_t DWT_TOTAL = (SYSTEM_US_TICKS * uSec);

    while((DWT->CYCCNT - DWT_START) < DWT_TOTAL);
}

void HAL_Core_System_Reset_Ex(int reason, uint32_t data, void *reserved) {
    WDG_InitTypeDef WDG_InitStruct;
    u32 CountProcess;
    u32 DivFacProcess;
    BKUP_Set(BKUP_REG0, BIT_KM4SYS_RESET_HAPPEN);
    WDG_Scalar(50, &CountProcess, &DivFacProcess);
    WDG_InitStruct.CountProcess = CountProcess;
    WDG_InitStruct.DivFacProcess = DivFacProcess;
    WDG_Init(&WDG_InitStruct);
    WDG_Cmd(ENABLE);
    DelayMs(500);
    // It should have reset the device after this amount of delay. If not, try resetting device by KM0
    km0_km4_ipc_send_request(KM0_KM4_IPC_CHANNEL_GENERIC, KM0_KM4_IPC_MSG_RESET, NULL, 0, NULL, NULL);
    while (1);
}

int HAL_Core_Enter_Panic_Mode(void* reserved) {
    __disable_irq();
    return 0;
}
