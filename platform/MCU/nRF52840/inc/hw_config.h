
#pragma once

/*
 * This header defines the platform-provided symbols needed to build a functioning
 * bootloader.
 */

#include "platform_config.h"
#include "platform_system_flags.h"
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
typedef enum
{
    BUTTON1 = 0, BUTTON1_MIRROR = 1
} Button_TypeDef;

typedef enum
{
    BUTTON_MODE_GPIO = 0, BUTTON_MODE_EXTI = 1
} ButtonMode_TypeDef;

typedef struct {
    uint16_t              pin;
    nrf_gpio_pin_dir_t    mode;
    nrf_gpio_pin_pull_t   pupd;
    __IO uint8_t          active;
    __IO uint16_t         debounce_time;
    uint16_t              event_in;
    uint16_t              event_channel;
    uint16_t              int_mask;
    nrf_gpiote_polarity_t int_trigger;
    uint16_t              nvic_irqn;
    uint16_t              nvic_irq_prio;
    uint8_t               padding[12];
} button_config_t;

extern button_config_t HAL_Buttons[];

extern uint8_t USE_SYSTEM_FLAGS;
extern uint16_t tempFlag;

/**
 * Access to named system flags.
 */

#define RESET                   0
#define RCC_FLAG_IWDGRST        0

inline int RCC_GetFlagStatus(int reg) { return RESET; }
inline void RCC_ClearFlag() {}

#define SYSTEM_FLAG(x) (system_flags.x)
void Load_SystemFlags(void);
void Save_SystemFlags(void);
extern platform_system_flags_t system_flags;

// these are functions used by the bootloader

inline void OTA_Finished_ResetStatus() {}
inline void OTA_Flashed_ResetStatus() {}
inline bool OTA_Flash_Reset() { return false; }
inline bool FACTORY_Flash_Reset() { return 0; }
inline void BACKUP_Flash_Reset() {}
void Finish_Update();
inline void Bootloader_Update_Version(int version) {}



#define __IO volatile
inline void IWDG_Reset_Enable(int count) {}
inline void SysTick_Disable() {}

inline void Save_Reset_Syndrome() {}

void Set_System(void);
void Reset_System(void);
void NVIC_Configuration(void);
void SysTick_Configuration(void);

void UI_Timer_Configure(void);

void BUTTON_Init(Button_TypeDef Button, ButtonMode_TypeDef Button_Mode);
void BUTTON_EXTI_Config(Button_TypeDef Button, FunctionalState NewState);
uint8_t BUTTON_GetState(Button_TypeDef Button);
uint16_t BUTTON_GetDebouncedTime(Button_TypeDef Button);
void BUTTON_ResetDebouncedState(Button_TypeDef Button);

void LED_Init(Led_TypeDef Led);

uint32_t Compute_CRC32(const uint8_t *pBuffer, uint32_t bufferSize);

#ifdef __cplusplus
}
#endif /* __cplusplus */
