#include "button.h"

extern __IO uint16_t BUTTON_DEBOUNCED_TIME[];

void BUTTON_Init_Ext() {
    if (BUTTON_Is_Pressed(BUTTON1))
        TIM_ITConfig(TIM1, TIM_IT_CC4, ENABLE);
}

uint8_t BUTTON_Is_Pressed(Button_TypeDef button) {
    return BUTTON_GetState(BUTTON1) == BUTTON1_PRESSED;
}

uint16_t BUTTON_Pressed_Time(Button_TypeDef button) {
    return BUTTON_DEBOUNCED_TIME[BUTTON1];
}
