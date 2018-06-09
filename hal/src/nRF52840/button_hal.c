#include "button_hal.h"
#include <nrf_rtc.h>
#include <nrf_nvic.h>
#include "platform_config.h"


button_config_t HAL_Buttons[] = {
    {
        .active = 0,
        .pin = BUTTON1_GPIO_PIN,
        .mode = BUTTON1_GPIO_MODE,
        .pupd = BUTTON1_GPIO_PUPD,
        .debounce_time = 0,

        .event_in = BUTTON1_GPIOTE_EVENT_IN,
        .event_channel = BUTTON1_GPIOTE_EVENT_CHANNEL,
        .int_mask = BUTTON1_GPIOTE_INT_MASK,
        .int_trigger = BUTTON1_GPIOTE_TRIGGER,
        .nvic_irqn = BUTTON1_GPIOTE_IRQn,
        .nvic_irq_prio = BUTTON1_GPIOTE_IRQ_PRIORITY
    },
    {
        .active = 0,
        .debounce_time = 0
    }
};
/**
 * @brief  Configures Button GPIO, EXTI Line and DEBOUNCE Timer.
 * @param  Button: Specifies the Button to be configured.
 *   This parameter can be one of following parameters:
 *     @arg BUTTON1: Button1
 * @param  Button_Mode: Specifies Button mode.
 *   This parameter can be one of following parameters:
 *     @arg BUTTON_MODE_GPIO: Button will be used as simple IO
 *     @arg BUTTON_MODE_EXTI: Button will be connected to EXTI line with interrupt
 *                     generation capability
 * @retval None
 */
void BUTTON_Init(Button_TypeDef Button, ButtonMode_TypeDef Button_Mode)
{
    nrf_gpio_cfg(
        HAL_Buttons[Button].pin,
        HAL_Buttons[Button].mode,
        NRF_GPIO_PIN_INPUT_CONNECT,
        HAL_Buttons[Button].pupd,
        NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_NOSENSE);

    if (Button_Mode == BUTTON_MODE_EXTI)
    {
        /* Disable RTC0 tick Interrupt */
        nrf_rtc_int_disable(NRF_RTC1, NRF_RTC_INT_TICK_MASK);

        BUTTON_EXTI_Config(Button, ENABLE);

        sd_nvic_SetPriority(HAL_Buttons[Button].nvic_irqn, HAL_Buttons[Button].nvic_irq_prio);
        sd_nvic_ClearPendingIRQ(HAL_Buttons[Button].nvic_irqn);
        sd_nvic_EnableIRQ(HAL_Buttons[Button].nvic_irqn);

        /* Enable the RTC0 NVIC Interrupt */
        sd_nvic_SetPriority(RTC1_IRQn, RTC1_IRQ_PRIORITY);
        sd_nvic_ClearPendingIRQ(RTC1_IRQn);
        sd_nvic_EnableIRQ(RTC1_IRQn);
    }
}

void BUTTON_EXTI_Config(Button_TypeDef Button, FunctionalState NewState)
{
    nrf_gpiote_event_configure(HAL_Buttons[Button].event_channel,
                               HAL_Buttons[Button].pin,
                               HAL_Buttons[Button].int_trigger);

    nrf_gpiote_event_clear(HAL_Buttons[Button].event_in);

    nrf_gpiote_event_enable(HAL_Buttons[Button].event_channel);

    if (NewState == ENABLE)
    {
        nrf_gpiote_int_enable(HAL_Buttons[Button].int_mask);
    }
    else
    {
        nrf_gpiote_int_disable(HAL_Buttons[Button].int_mask);
    }
}

/**
 * @brief  Returns the selected Button non-filtered state.
 * @param  Button: Specifies the Button to be checked.
 *   This parameter can be one of following parameters:
 *     @arg BUTTON1: Button1
 * @retval Actual Button Pressed state.
 */
uint8_t BUTTON_GetState(Button_TypeDef Button)
{
    return nrf_gpio_pin_read(HAL_Buttons[Button].pin);
}

/**
 * @brief  Returns the selected Button Debounced Time.
 * @param  Button: Specifies the Button to be checked.
 *   This parameter can be one of following parameters:
 *     @arg BUTTON1: Button1
 * @retval Button Debounced time in millisec.
 */
uint16_t BUTTON_GetDebouncedTime(Button_TypeDef Button)
{
    return HAL_Buttons[Button].debounce_time;
}

void BUTTON_ResetDebouncedState(Button_TypeDef Button)
{
    HAL_Buttons[Button].debounce_time = 0;
}
