#include "button.h"
#include "dct.h"
#include "hal_irq_flag.h"
#include <string.h>

/**
 * @brief  This function handles BUTTON EXTI Handler.
 * @param  None
 * @retval None
 */
void hal_button_irq_handler(uint16_t exti)
{
    if (EXTI_GetITStatus(exti) != RESET)
    {
        /* Clear the EXTI line pending bit */
        EXTI_ClearITPendingBit(exti);

        hal_button_check_irq(HAL_BUTTON1, exti);
        hal_button_check_irq(HAL_BUTTON1_MIRROR, exti);
    }
}

void hal_button_check_irq(uint16_t button, uint16_t exti) {
    if (HAL_Buttons[button].exti_line == exti)
    {
        HAL_Buttons[button].debounce_time = 0x00;
        HAL_Buttons[button].active = 1;

        /* Disable button Interrupt */
        hal_button_exti_config(button, DISABLE);

        /* Enable TIM2 CC1 Interrupt */
        TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);
    }
}

void hal_button_check_state(uint16_t button, uint8_t pressed) {
    if (HAL_Buttons[button].exti_line && hal_button_get_state(button) == pressed)
    {
        if (!HAL_Buttons[button].active)
            HAL_Buttons[button].active = 1;
        HAL_Buttons[button].debounce_time += BUTTON_DEBOUNCE_INTERVAL;
    }
    else if (HAL_Buttons[button].active)
    {
        HAL_Buttons[button].active = 0;
        /* Enable button Interrupt */
        hal_button_exti_config(button, ENABLE);
    }
}

int hal_button_debounce() {
    hal_button_check_state(HAL_BUTTON1, BUTTON1_PRESSED);
    hal_button_check_state(HAL_BUTTON1_MIRROR, HAL_Buttons[HAL_BUTTON1_MIRROR].exti_trigger == EXTI_Trigger_Rising ? 1 : 0);

    int pressed = HAL_Buttons[HAL_BUTTON1].active + HAL_Buttons[HAL_BUTTON1_MIRROR].active;
    if (pressed == 0) {
        /* Disable TIM2 CC1 Interrupt */
        TIM_ITConfig(TIM2, TIM_IT_CC1, DISABLE);
    }

    return pressed;
}

void hal_button_init_ext() {
    const hal_button_config_t* conf = (const hal_button_config_t*)dct_read_app_data_lock(DCT_MODE_BUTTON_MIRROR_OFFSET);

    if (conf && conf->active == 0xAA && conf->debounce_time == 0xBBCC) {
        //int32_t state = HAL_disable_irq();
        memcpy((void*)&HAL_Buttons[HAL_BUTTON1_MIRROR], (void*)conf, sizeof(hal_button_config_t));
        HAL_Buttons[HAL_BUTTON1_MIRROR].active = 0;
        HAL_Buttons[HAL_BUTTON1_MIRROR].debounce_time = 0;
        hal_button_init(HAL_BUTTON1_MIRROR, HAL_BUTTON_MODE_EXTI);
        //HAL_enable_irq(state);
    }

    dct_read_app_data_unlock(DCT_MODE_BUTTON_MIRROR_OFFSET);

    if (hal_button_debounce())
        TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);
}

uint8_t hal_button_is_pressed(hal_button_t button) {
    uint8_t pressed = 0;
    pressed = HAL_Buttons[button].active;

    if (button == HAL_BUTTON1 && HAL_Buttons[HAL_BUTTON1_MIRROR].exti_line) {
        pressed |= hal_button_is_pressed(HAL_BUTTON1_MIRROR);
    }

    return pressed;
}

uint16_t hal_button_get_pressed_time(hal_button_t button) {
    uint16_t pressed = 0;

    pressed = HAL_Buttons[button].debounce_time;
    if (button == HAL_BUTTON1 && HAL_Buttons[HAL_BUTTON1_MIRROR].exti_line) {
        if (hal_button_get_pressed_time(HAL_BUTTON1_MIRROR) > pressed)
            pressed = hal_button_get_pressed_time(HAL_BUTTON1_MIRROR);
    }

    return pressed;
}
