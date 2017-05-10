#include "system_flags_impl.h"

#include "stm32f2xx_rtc.h"

#include <string.h>

#define SYSTEM_FLAGS_MAGIC_NUMBER 0x1ADEACC0u

int Load_SystemFlags_Impl(platform_system_flags_t* f) {
/*
    System flags layout:

                |      31 .. 24      |      23 .. 16      |      15 .. 08      |      07 .. 00
    RTC_BKP_DR4 | Magic number
    RTC_BKP_DR5 | NVMEM_SPARK_Reset                       | Bootloader_Version
    RTC_BKP_DR6 | OTA_FLASHED_Status                      | FLASH_OTA_Update
    RTC_BKP_DR7 | IWDG_Enable                             | Factory_Reset
    RTC_BKP_DR8 | FeaturesEnabled    | StartupMode        | Factory_Reset_Done | dfu_on_no_firmware
    RTC_BKP_DR9 | RCC_CSR
*/
    // Magic number
    uint32_t v = RTC_ReadBackupRegister(RTC_BKP_DR4);
    if (v != SYSTEM_FLAGS_MAGIC_NUMBER) {
        // Fill flags structure with 0xff to preserve compatibility with existing code
        memset(f, 0xff, sizeof(platform_system_flags_t));
        f->header = SYSTEM_FLAGS_MAGIC_NUMBER;
        return -1; // Not an error technically
    }
    f->header = SYSTEM_FLAGS_MAGIC_NUMBER;
    // Bootloader_Version_SysFlag, NVMEM_SPARK_Reset_SysFlag
    v = RTC_ReadBackupRegister(RTC_BKP_DR5);
    f->Bootloader_Version_SysFlag = v & 0xffff;
    f->NVMEM_SPARK_Reset_SysFlag = (v >> 16) & 0xffff;
    // FLASH_OTA_Update_SysFlag, OTA_FLASHED_Status_SysFlag
    v = RTC_ReadBackupRegister(RTC_BKP_DR6);
    f->FLASH_OTA_Update_SysFlag = v & 0xffff;
    f->OTA_FLASHED_Status_SysFlag = (v >> 16) & 0xffff;
    // Factory_Reset_SysFlag, IWDG_Enable_SysFlag
    v = RTC_ReadBackupRegister(RTC_BKP_DR7);
    f->Factory_Reset_SysFlag = v & 0xffff;
    f->IWDG_Enable_SysFlag = (v >> 16) & 0xffff;
    // dfu_on_no_firmware, Factory_Reset_Done_SysFlag, StartupMode_SysFlag, FeaturesEnabled_SysFlag
    v = RTC_ReadBackupRegister(RTC_BKP_DR8);
    f->dfu_on_no_firmware = v & 0xff;
    f->Factory_Reset_Done_SysFlag = (v >> 8) & 0xff;
    f->StartupMode_SysFlag = (v >> 16) & 0xff;
    f->FeaturesEnabled_SysFlag = (v >> 24) & 0xff;
    // RCC_CSR_SysFlag
    v = RTC_ReadBackupRegister(RTC_BKP_DR9);
    f->RCC_CSR_SysFlag = v;
    return 0;
}

int Save_SystemFlags_Impl(const platform_system_flags_t* f) {
    // Don't update system flags with uninitialized data
    if (f->header != SYSTEM_FLAGS_MAGIC_NUMBER) {
        return -1;
    }
    // Bootloader_Version_SysFlag, NVMEM_SPARK_Reset_SysFlag
    uint32_t v = (((uint32_t)f->NVMEM_SPARK_Reset_SysFlag & 0xffff) << 16) | ((uint32_t)f->Bootloader_Version_SysFlag & 0xffff);
    RTC_WriteBackupRegister(RTC_BKP_DR5, v);
    // FLASH_OTA_Update_SysFlag, OTA_FLASHED_Status_SysFlag
    v = (((uint32_t)f->OTA_FLASHED_Status_SysFlag & 0xffff) << 16) | ((uint32_t)f->FLASH_OTA_Update_SysFlag & 0xffff);
    RTC_WriteBackupRegister(RTC_BKP_DR6, v);
    // Factory_Reset_SysFlag, IWDG_Enable_SysFlag
    v = (((uint32_t)f->IWDG_Enable_SysFlag & 0xffff) << 16) | ((uint32_t)f->Factory_Reset_SysFlag & 0xffff);
    RTC_WriteBackupRegister(RTC_BKP_DR7, v);
    // dfu_on_no_firmware, Factory_Reset_Done_SysFlag, StartupMode_SysFlag, FeaturesEnabled_SysFlag
    v = (((uint32_t)f->FeaturesEnabled_SysFlag & 0xff) << 24) | (((uint32_t)f->StartupMode_SysFlag & 0xff) << 16) |
            (((uint32_t)f->Factory_Reset_Done_SysFlag & 0xff) << 8) | ((uint32_t)f->dfu_on_no_firmware & 0xff);
    RTC_WriteBackupRegister(RTC_BKP_DR8, v);
    // RCC_CSR_SysFlag
    v = f->RCC_CSR_SysFlag;
    RTC_WriteBackupRegister(RTC_BKP_DR9, v);
    // Magic number
    v = SYSTEM_FLAGS_MAGIC_NUMBER;
    RTC_WriteBackupRegister(RTC_BKP_DR4, v);
    return 0;
}
