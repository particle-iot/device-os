#include "button.h"
#include "dct.h"
#include "hal_irq_flag.h"
#include <string.h>

/**
 * @brief  This function handles BUTTON EXTI Handler.
 * @param  None
 * @retval None
 */
void BUTTON_Irq_Handler(uint16_t exti)
{
    if (EXTI_GetITStatus(exti) != RESET)
    {
        /* Clear the EXTI line pending bit */
        EXTI_ClearITPendingBit(exti);

        BUTTON_Check_Irq(BUTTON1, exti);
        BUTTON_Check_Irq(BUTTON1_MIRROR, exti);
    }
}

void BUTTON_Check_Irq(uint16_t button, uint16_t exti) {
    if (HAL_Buttons[button].exti_line == exti)
    {
        HAL_Buttons[button].debounce_time = 0x00;
        HAL_Buttons[button].active = 1;

        /* Disable button Interrupt */
        BUTTON_EXTI_Config(button, DISABLE);

        /* Enable TIM2 CC1 Interrupt */
        TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);
    }
}

void BUTTON_Check_State(uint16_t button, uint8_t pressed) {
    if (HAL_Buttons[button].exti_line && BUTTON_GetState(button) == pressed)
    {
        if (!HAL_Buttons[button].active)
            HAL_Buttons[button].active = 1;
        HAL_Buttons[button].debounce_time += BUTTON_DEBOUNCE_INTERVAL;
    }
    else if (HAL_Buttons[button].active)
    {
        HAL_Buttons[button].active = 0;
        /* Enable button Interrupt */
        BUTTON_EXTI_Config(button, ENABLE);
    }
}

int BUTTON_Debounce() {
    BUTTON_Check_State(BUTTON1, BUTTON1_PRESSED);
    BUTTON_Check_State(BUTTON1_MIRROR, HAL_Buttons[BUTTON1_MIRROR].exti_trigger == EXTI_Trigger_Rising ? 1 : 0);

    int pressed = HAL_Buttons[BUTTON1].active + HAL_Buttons[BUTTON1_MIRROR].active;
    if (pressed == 0) {
        /* Disable TIM2 CC1 Interrupt */
        TIM_ITConfig(TIM2, TIM_IT_CC1, DISABLE);
    }

    return pressed;
}

void BUTTON_Init_Ext() {
    const button_config_t* conf = (const button_config_t*)dct_read_app_data_lock(DCT_MODE_BUTTON_MIRROR_OFFSET);

    if (conf && conf->active == 0xAA && conf->debounce_time == 0xBBCC) {
        //int32_t state = HAL_disable_irq();
        memcpy((void*)&HAL_Buttons[BUTTON1_MIRROR], (void*)conf, sizeof(button_config_t));
        HAL_Buttons[BUTTON1_MIRROR].active = 0;
        HAL_Buttons[BUTTON1_MIRROR].debounce_time = 0;
        BUTTON_Init(BUTTON1_MIRROR, BUTTON_MODE_EXTI);
        //HAL_enable_irq(state);
    }

    dct_read_app_data_unlock(DCT_MODE_BUTTON_MIRROR_OFFSET);

    if (BUTTON_Debounce())
        TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);
}

uint8_t BUTTON_Is_Pressed(Button_TypeDef button) {
    uint8_t pressed = 0;
    pressed = HAL_Buttons[button].active;

    if (button == BUTTON1 && HAL_Buttons[BUTTON1_MIRROR].exti_line) {
        pressed |= BUTTON_Is_Pressed(BUTTON1_MIRROR);
    }

    return pressed;
}

uint16_t BUTTON_Pressed_Time(Button_TypeDef button) {
    uint16_t pressed = 0;

    pressed = HAL_Buttons[button].debounce_time;
    if (button == BUTTON1 && HAL_Buttons[BUTTON1_MIRROR].exti_line) {
        if (BUTTON_Pressed_Time(BUTTON1_MIRROR) > pressed)
            pressed = BUTTON_Pressed_Time(BUTTON1_MIRROR);
    }

    return pressed;
}
