#include "button.h"

void BUTTON_Irq_Handler(void)
{
    if (nrf_gpiote_event_is_set(HAL_Buttons[BUTTON1].event_in))
    {
        nrf_gpiote_event_clear(HAL_Buttons[BUTTON1].event_in);

        BUTTON_Check_Irq(BUTTON1);
    }
}

void BUTTON_Check_Irq(uint16_t button)
{
    HAL_Buttons[button].debounce_time = 0x00;
    HAL_Buttons[button].active = 1;

    /* Disable button Interrupt */
    BUTTON_EXTI_Config(button, DISABLE);

    /* Enable RTC0 tick interrupt */
    nrf_rtc_int_enable(NRF_RTC0, NRF_RTC_INT_TICK_MASK);
}

void BUTTON_Check_State(uint16_t button, uint8_t pressed)
{
    if (BUTTON_GetState(button) == pressed)
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

int BUTTON_Debounce()
{
    BUTTON_Check_State(BUTTON1, BUTTON1_PRESSED);

    int pressed = HAL_Buttons[BUTTON1].active;
    if (pressed == 0)
    {
        /* Disable RTC0 tick Interrupt */
        nrf_rtc_int_disable(NRF_RTC0, NRF_RTC_INT_TICK_MASK);
    }

    return pressed;
}

void BUTTON_Init_Ext()
{
    if (BUTTON_Debounce())
        nrf_rtc_int_enable(NRF_RTC0, NRF_RTC_INT_TICK_MASK);
}

uint8_t BUTTON_Is_Pressed(Button_TypeDef button)
{
    return HAL_Buttons[button].active;
}

uint16_t BUTTON_Pressed_Time(Button_TypeDef button)
{
    return HAL_Buttons[button].debounce_time;
}
