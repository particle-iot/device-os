
#pragma once

/*
 * This header defines the platform-provided symbols needed to build a functioning
 * bootloader.
 */

#include "platform_config.h"
#include "module_info.h"
#include "module_info_hal.h"
#include "flash_device_hal.h"
#include "flash_mal.h"
#include <stdint.h>
#include <stdbool.h>


/* Exported types ------------------------------------------------------------*/
typedef enum
{
	BUTTON1 = 0, BUTTON2 = 1, BUTTON1_MIRROR = 2
} Button_TypeDef;

typedef enum
{
	BUTTON_MODE_GPIO = 0, BUTTON_MODE_EXTI = 1
} ButtonMode_TypeDef;


inline void Set_System() {}
inline void SysTick_Configuration() {}



extern uint8_t USE_SYSTEM_FLAGS;
extern uint16_t tempFlag;

/**
 * Access to named system flags.
 */
#define SYSTEM_FLAG(flag)       tempFlag

#define RESET                   0
#define RCC_FLAG_IWDGRST        0

inline int RCC_GetFlagStatus(int reg) { return RESET; }
inline void RCC_ClearFlag() {}

inline void Load_SystemFlags() {}
inline void Save_SystemFlags() {}

// these are functions used by the bootloader

inline void OTA_Finished_ResetStatus() {}
inline void OTA_Flashed_ResetStatus() {}
inline bool OTA_Flash_Reset() { return false; }
inline bool FACTORY_Flash_Reset() { return 0; }
inline void BACKUP_Flash_Reset() {}
inline void NVIC_SystemReset() {}
inline void Finish_Update() {}
inline void Bootloader_Update_Version(int version) {}



#define __IO volatile
inline void __set_MSP(uint32_t v) {}
inline void IWDG_Reset_Enable(int count) {}
inline void SysTick_Disable() {}

inline void Save_Reset_Syndrome() {}

