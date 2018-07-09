#ifndef _BUTTON_HAL_H
#define _BUTTON_HAL_H

#include <stdint.h>
#include "button_hal_impl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BUTTONn                             1

typedef enum {
    BUTTON1 = 0, 
    BUTTON1_MIRROR = 1
} Button_TypeDef;

typedef enum {
    BUTTON_MODE_GPIO = 0, 
    BUTTON_MODE_EXTI = 1
} ButtonMode_TypeDef;

void BUTTON_Init(Button_TypeDef Button, ButtonMode_TypeDef Button_Mode);
void BUTTON_Uninit();
void BUTTON_EXTI_Config(Button_TypeDef Button, FunctionalState NewState);
uint8_t BUTTON_GetState(Button_TypeDef Button);
uint16_t BUTTON_GetDebouncedTime(Button_TypeDef Button);
void BUTTON_ResetDebouncedState(Button_TypeDef Button);

void BUTTON_Init_Ext();
uint8_t BUTTON_Is_Pressed(Button_TypeDef button);
uint16_t BUTTON_Pressed_Time(Button_TypeDef button);

void BUTTON_Irq_Handler(void);
void BUTTON_Check_Irq(uint16_t button);
void BUTTON_Check_State(uint16_t button, uint8_t pressed);
int BUTTON_Debounce();

extern button_config_t HAL_Buttons[];

#ifdef __cplusplus
}
#endif

#endif