#include "rgbled_hal.h"
#include "rgbled_hal_impl.h"
#include "rgbled.h"
#include "pinmap_hal.h"
#include "pwm_hal.h"
#include "gpio_hal.h"
#include "hw_config.h"

static struct LedSetCallbacksInitializer {
    LedSetCallbacksInitializer() {
        LED_SetCallbacks({
            0x0001,
            HAL_Led_Rgb_Set_Values,
            HAL_Led_Rgb_Get_Values,
            HAL_Led_Rgb_Get_Max_Value,
            HAL_Led_User_Set,
            HAL_Led_User_Toggle
        }, NULL);
    }
} s_LedSetCallbacksInitializer;

led_config_t HAL_Leds[] = {
    // Normal LEDs are defined in HAL_Leds_Default

    // Mirrored LEDs
    {0},
    {0},
    {0},
    {0}
};

static void Set_LED_Value(Led_TypeDef led, uint16_t value)
{
    led_config_t* ledc = HAL_Led_Get_Configuration(led, NULL);
    const uint32_t pwmMax = (((uint32_t)1 << 15) - 1);
    uint32_t corrected = (uint32_t)value * pwmMax / Get_RGB_LED_Max_Value();
    if (ledc->is_hal_pin && ledc->is_active && ledc->is_pwm) {
        HAL_PWM_Write_Ext(ledc->hal_pin, ledc->is_inverted ? (pwmMax - corrected) : corrected);
    }
}

static void Set_LED_State(Led_TypeDef led, uint8_t state)
{
    led_config_t* ledc = HAL_Led_Get_Configuration(led, NULL);
    if (state)
        ledc->port->BSRRL = ledc->pin;
    else
        ledc->port->BSRRH = ledc->pin;
}

static void Toggle_LED_State(Led_TypeDef led)
{
    led_config_t* ledc = HAL_Led_Get_Configuration(led, NULL);
    ledc->port->ODR ^= ledc->pin;
}

static void LED_Init_Hal(Led_TypeDef led)
{
    led_config_t* ledc = HAL_Led_Get_Configuration(led, NULL);
    HAL_Pin_Mode(ledc->hal_pin, (PinMode)ledc->hal_mode);
    if (ledc->hal_mode == AF_OUTPUT_PUSHPULL) {
        // Enable PWM
        // 15-bit resolution is supported by all PWM pins on STM32F2
        ledc->is_pwm = 1;
        HAL_PWM_Set_Resolution(ledc->hal_pin, 15);
        // Set to maximum
        Set_LED_Value(led, Get_RGB_LED_Max_Value());
    } else {
        ledc->is_pwm = 0;
        Set_LED_State(led, ledc->is_inverted ? ENABLE : DISABLE);
    }
}

void HAL_Led_Rgb_Set_Values(uint16_t r, uint16_t g, uint16_t b, void* reserved)
{
    Set_RGB_LED_Values(r, g, b);

    Set_LED_Value((Led_TypeDef)(LED_RED   + LED_MIRROR_OFFSET), r);
    Set_LED_Value((Led_TypeDef)(LED_GREEN + LED_MIRROR_OFFSET), g);
    Set_LED_Value((Led_TypeDef)(LED_BLUE  + LED_MIRROR_OFFSET), b);
}

void HAL_Led_Rgb_Get_Values(uint16_t* rgb, void* reserved)
{
    Get_RGB_LED_Values(rgb);
}

uint32_t HAL_Led_Rgb_Get_Max_Value(void* reserved)
{
    return Get_RGB_LED_Max_Value();
}

void HAL_Led_User_Set(uint8_t state, void* reserved)
{
    Set_LED_State(LED_USER, state);
}

void HAL_Led_User_Toggle(void* reserved)
{
    Toggle_LED_State(LED_USER);
}

led_config_t* HAL_Led_Set_Configuration(uint8_t led, led_config_t* conf, void* reserved)
{
    if (led < LED_MIRROR_OFFSET)
        return NULL;
    HAL_Leds[led - LED_MIRROR_OFFSET] = *conf;
    return &HAL_Leds[led - LED_MIRROR_OFFSET];
}

led_config_t* HAL_Led_Get_Configuration(uint8_t led, void* reserved)
{
    if (led < LED_MIRROR_OFFSET)
        return (led_config_t*)&HAL_Leds_Default[led];
    return &HAL_Leds[led - LED_MIRROR_OFFSET];
}

void HAL_Led_Init(uint8_t led, led_config_t* conf, void* reserved)
{
    if (led < LED_MIRROR_OFFSET)
    {
        LED_Init((Led_TypeDef)led);
        return;
    }

    if (conf) {
        HAL_Led_Set_Configuration(led, conf, NULL);
    }
    LED_Init_Hal((Led_TypeDef)led);
}
