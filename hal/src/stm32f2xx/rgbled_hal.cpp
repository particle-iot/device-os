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
            hal_led_set_rgb_values,
            hal_led_get_rgb_values,
            hal_led_get_max_rgb_values,
            hal_led_set_user,
            hal_led_toggle_user
        }, NULL);
    }
} s_LedSetCallbacksInitializer;

hal_led_config_t HAL_Leds[] = {
    // Normal LEDs are defined in HAL_Leds_Default

    // Mirrored LEDs
    {0},
    {0},
    {0},
    {0}
};

static void Set_LED_Value(Led_TypeDef led, uint16_t value)
{
    hal_led_config_t* ledc = hal_led_get_configuration(led, NULL);
    const uint32_t pwmMax = (((uint32_t)1 << 15) - 1);
    uint32_t corrected = (uint32_t)value * pwmMax / Get_RGB_LED_Max_Value();
    if (ledc->is_hal_pin && ledc->is_active && ledc->is_pwm) {
        hal_pwm_write_ext(ledc->hal_pin, ledc->is_inverted ? (pwmMax - corrected) : corrected);
    }
}

static void Set_LED_State(Led_TypeDef led, uint8_t state)
{
    hal_led_config_t* ledc = hal_led_get_configuration(led, NULL);
    if (state)
        ledc->port->BSRRL = ledc->pin;
    else
        ledc->port->BSRRH = ledc->pin;
}

static void Toggle_LED_State(Led_TypeDef led)
{
    hal_led_config_t* ledc = hal_led_get_configuration(led, NULL);
    ledc->port->ODR ^= ledc->pin;
}

static void LED_Init_Hal(Led_TypeDef led)
{
    hal_led_config_t* ledc = hal_led_get_configuration(led, NULL);
    hal_gpio_mode(ledc->hal_pin, (PinMode)ledc->hal_mode);
    if (ledc->hal_mode == AF_OUTPUT_PUSHPULL) {
        // Enable PWM
        // 15-bit resolution is supported by all PWM pins on STM32F2
        ledc->is_pwm = 1;
        hal_pwm_set_resolution(ledc->hal_pin, 15);
        // Set to maximum
        Set_LED_Value(led, Get_RGB_LED_Max_Value());
    } else {
        ledc->is_pwm = 0;
        Set_LED_State(led, ledc->is_inverted ? ENABLE : DISABLE);
    }
}

void hal_led_set_rgb_values(uint16_t r, uint16_t g, uint16_t b, void* reserved)
{
    Set_RGB_LED_Values(r, g, b);

    Set_LED_Value((Led_TypeDef)(PARTICLE_LED_RED   + LED_MIRROR_OFFSET), r);
    Set_LED_Value((Led_TypeDef)(PARTICLE_LED_GREEN + LED_MIRROR_OFFSET), g);
    Set_LED_Value((Led_TypeDef)(PARTICLE_LED_BLUE  + LED_MIRROR_OFFSET), b);
}

void hal_led_get_rgb_values(uint16_t* rgb, void* reserved)
{
    Get_RGB_LED_Values(rgb);
}

uint32_t hal_led_get_max_rgb_values(void* reserved)
{
    return Get_RGB_LED_Max_Value();
}

void hal_led_set_user(uint8_t state, void* reserved)
{
    Set_LED_State(PARTICLE_LED_USER, state);
}

void hal_led_toggle_user(void* reserved)
{
    Toggle_LED_State(PARTICLE_LED_USER);
}

hal_led_config_t* hal_led_set_configuration(uint8_t led, hal_led_config_t* conf, void* reserved)
{
    if (led < LED_MIRROR_OFFSET)
        return NULL;
    HAL_Leds[led - LED_MIRROR_OFFSET] = *conf;
    return &HAL_Leds[led - LED_MIRROR_OFFSET];
}

hal_led_config_t* hal_led_get_configuration(uint8_t led, void* reserved)
{
    if (led < LED_MIRROR_OFFSET)
        return (hal_led_config_t*)&HAL_Leds_Default[led];
    return &HAL_Leds[led - LED_MIRROR_OFFSET];
}

void hal_led_init(uint8_t led, hal_led_config_t* conf, void* reserved)
{
    if (led < LED_MIRROR_OFFSET)
    {
        LED_Init((Led_TypeDef)led);
        return;
    }

    if (conf) {
        hal_led_set_configuration(led, conf, NULL);
    }
    LED_Init_Hal((Led_TypeDef)led);
}
