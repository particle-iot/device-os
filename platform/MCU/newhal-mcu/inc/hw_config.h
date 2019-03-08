
#pragma once

/*
 * This header defines the platform-provided symbols needed to build a functioning
 * bootloader.
 */

#include "module_info.h"
#include "module_info_hal.h"
#include "flash_device_hal.h"
#include <stdint.h>
#include <stdbool.h>

inline const module_info_t* FLASH_ModuleInfo(flash_device_t device, uint32_t address) { return (module_info_t*)0; }

inline void Set_System() {}
inline void SysTick_Configuration() {}



extern uint8_t USE_SYSTEM_FLAGS;
extern uint16_t tempFlag;

/**
 * Access to named system flags.
 */
#define SYSTEM_FLAG(flag)       tempFlag

#define CORE_FW_ADDRESS         0

#define RESET                   0
#define RCC_FLAG_IWDGRST        0

inline int RCC_GetFlagStatus(int reg) { return RESET; }
inline void RCC_ClearFlag() {}

inline void Load_SystemFlags() {}
inline void Save_SystemFlags() {}

// these are functions used by the bootloader

inline void FLASH_Erase() {}
inline void OTA_Finished_ResetStatus() {}
inline void OTA_Flashed_ResetStatus() {}
inline bool OTA_Flash_Reset() { return false; }
inline bool FACTORY_Flash_Reset() { return 0; }
inline void BACKUP_Flash_Reset() {}
inline void NVIC_SystemReset() {}
inline void Finish_Update() {}
inline void Bootloader_Update_Version(int version) {}

inline int BUTTON_GetState(int button) { return 0; }


#define __IO volatile
inline void __set_MSP(uint32_t v) {}
inline void IWDG_Reset_Enable(int count) {}
inline void SysTick_Disable() {}

inline void Save_Reset_Syndrome() {}

bool FLASH_IsFactoryResetAvailable(void);
