#ifndef __BUTTON_H
#define __BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "hw_config.h"

void BUTTON_Init_Ext();

uint8_t BUTTON_Is_Pressed(Button_TypeDef button);
uint16_t BUTTON_Pressed_Time(Button_TypeDef button);


void BUTTON_Irq_Handler(uint16_t exti);
void BUTTON_Check_Irq(uint16_t button, uint16_t exti);
void BUTTON_Check_State(uint16_t button, uint8_t pressed);
int BUTTON_Debounce();

#ifdef __cplusplus
}
#endif

#endif /* __BUTTON_H */
