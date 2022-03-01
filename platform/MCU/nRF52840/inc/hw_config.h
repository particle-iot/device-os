
#pragma once

/*
 * This header defines the platform-provided symbols needed to build a functioning
 * bootloader.
 */

#include "platform_config.h"
#include "platform_system_flags.h"
#include "hw_system_flags.h"
#include "module_info.h"
#include "module_info_hal.h"
#include "flash_device_hal.h"
#include "flash_mal.h"
#include <stdint.h>
#include <stdbool.h>
#include "hw_ticks.h"
#include "rgbled.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Exported types ------------------------------------------------------------*/
extern uint8_t USE_SYSTEM_FLAGS;
extern uint16_t tempFlag;


#define SYSTEM_FLAG(x) (system_flags.x)
void Load_SystemFlags(void);
void Save_SystemFlags(void);
extern platform_system_flags_t system_flags;

// these are functions used by the bootloader

void OTA_Flash_Reset(void);
bool FACTORY_Flash_Reset(void);
void BACKUP_Flash_Reset(void);

#define __IO volatile
void IWDG_Reset_Enable(uint32_t msTimeout);
void SysTick_Disable();

void Set_System(void);
void Reset_System(void);
void NVIC_Configuration(void);
void SysTick_Configuration(void);

void UI_Timer_Configure(void);


bool OTA_Flashed_GetStatus(void);
void OTA_Flashed_ResetStatus(void);

void Finish_Update(void);

uint16_t Bootloader_Get_Version(void);
void Bootloader_Update_Version(uint16_t bootloaderVersion);

uint32_t Compute_CRC32(const uint8_t *pBuffer, uint32_t bufferSize, uint32_t const *p_crc);


#ifdef __cplusplus
}
#endif /* __cplusplus */
