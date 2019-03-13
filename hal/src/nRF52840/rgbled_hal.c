#include "rgbled_hal.h"
#include "rgbled.h"
#include "module_info.h"
#include "platform_config.h"
#include "dct.h"
#include "interrupts_hal.h"
#include "gpio_hal.h"
#include "pwm_hal.h"
#include "pinmap_impl.h"

led_config_t HAL_Leds[LEDn] = {
    {
        .version = LED_CONFIG_STRUCT_VERSION,
        .pin = LED_PIN_USER,
        .is_inverted = 0,
        .is_active = 1
    },
    {
        .version = LED_CONFIG_STRUCT_VERSION,
        .pin = LED_PIN_RED,
        .is_inverted = 1,
        .is_active = 1
    },
    {
        .version = LED_CONFIG_STRUCT_VERSION,
        .pin = LED_PIN_GREEN,
        .is_inverted = 1,
        .is_active = 1
    },
    {
        .version = LED_CONFIG_STRUCT_VERSION,
        .pin = LED_PIN_BLUE,
        .is_inverted = 1,
        .is_active = 1
    },
};

static void set_led_value(Led_TypeDef led, uint16_t value) {
    // mirror led configuration is changed, should reconfigure mirror LED
    if (HAL_Leds[led].version != LED_CONFIG_STRUCT_VERSION) {
        return;
    }

    if (HAL_Leds[led].is_active) {
        uint32_t pwm_max = Get_RGB_LED_Max_Value();
        HAL_PWM_Write_Ext(HAL_Leds[led].pin, HAL_Leds[led].is_inverted ? (pwm_max - value) : value);
    }
}

/**
 * @brief  Set RGB LED value
 */
void Set_RGB_LED_Values(uint16_t r, uint16_t g, uint16_t b) {
    set_led_value(LED_RED, r);
    set_led_value(LED_GREEN, g);
    set_led_value(LED_BLUE, b);

    set_led_value(LED_RED + LED_MIRROR_OFFSET, r);
    set_led_value(LED_GREEN + LED_MIRROR_OFFSET, g);
    set_led_value(LED_BLUE + LED_MIRROR_OFFSET, b);
}

/**
 * @brief  Get RGB LED current value
 */
void Get_RGB_LED_Values(uint16_t* values) {
    uint32_t pwm_max = Get_RGB_LED_Max_Value();

    values[0] = HAL_Leds[LED_RED].is_inverted ?
                (pwm_max - HAL_PWM_Get_AnalogValue(HAL_Leds[LED_RED].pin)) :
                HAL_PWM_Get_AnalogValue(HAL_Leds[LED_RED].pin);
    values[1] = HAL_Leds[LED_GREEN].is_inverted ?
                (pwm_max - HAL_PWM_Get_AnalogValue(HAL_Leds[LED_GREEN].pin)) :
                HAL_PWM_Get_AnalogValue(HAL_Leds[LED_GREEN].pin);
    values[2] = HAL_Leds[LED_BLUE].is_inverted ?
                (pwm_max - HAL_PWM_Get_AnalogValue(HAL_Leds[LED_BLUE].pin)) :
                HAL_PWM_Get_AnalogValue(HAL_Leds[LED_BLUE].pin);
}

/**
 * @brief  Get RGB LED max value
 */
uint16_t Get_RGB_LED_Max_Value(void) {
    return (1 << HAL_PWM_Get_Resolution(HAL_Leds[LED_RED].pin)) - 1;
}

/**
 * @brief  Configure user LED
 */
void Set_User_LED(uint8_t state) {
    if ((!state && HAL_Leds[LED_USER].is_inverted) || (state && !HAL_Leds[LED_USER].is_inverted)) {
        HAL_GPIO_Write(HAL_Leds[LED_USER].pin, 1);
    } else {
        HAL_GPIO_Write(HAL_Leds[LED_USER].pin, 0);
    }
}

/**
 * @brief  Toggle user LED
 */
void Toggle_User_LED(void) {
    HAL_GPIO_Write(HAL_Leds[LED_USER].pin, !HAL_GPIO_Read(HAL_Leds[LED_USER].pin));
}

/**
 * @brief  Configures LED GPIO
 */
void LED_Init(Led_TypeDef Led) {
#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
    if (Led >= LED_MIRROR_OFFSET) {
        // Load configuration from DCT
        led_config_t conf;
        const size_t offset = DCT_LED_MIRROR_OFFSET + ((Led - LED_MIRROR_OFFSET) * sizeof(led_config_t));
        if (dct_read_app_data_copy(offset, &conf, sizeof(conf)) == 0 &&
            conf.version != 0xff &&
            conf.is_active) {
            //int32_t state = HAL_disable_irq();
            memcpy((void*)&HAL_Leds[Led], (void*)&conf, sizeof(led_config_t));
            //HAL_enable_irq(state);
        } else {
            return;
        }
    }
#endif // MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
    if (HAL_Leds[Led].version != LED_CONFIG_STRUCT_VERSION) {
        return;
    }

    HAL_Pin_Mode(HAL_Leds[Led].pin, OUTPUT);
    if (HAL_Leds[Led].is_inverted) {
        HAL_GPIO_Write(HAL_Leds[Led].pin, 1);
    } else {
        HAL_GPIO_Write(HAL_Leds[Led].pin, 0);
    }
}

/**
 * @brief  Configures default RGB and mirror RGB GPIO
 */
void HAL_Led_Rgb_Set_Values(uint16_t r, uint16_t g, uint16_t b, void* reserved) {
    Set_RGB_LED_Values(r, g, b);
}

/**
 * @brief  Get current RGB value.
 */
void HAL_Led_Rgb_Get_Values(uint16_t* rgb, void* reserved) {
    Get_RGB_LED_Values(rgb);
}

/**
 * @brief  Get PWM max value of RGB LED.
 */
uint32_t HAL_Led_Rgb_Get_Max_Value(void* reserved) {
    return Get_RGB_LED_Max_Value();
}

/**
 * @brief  Set user LED
 */
void HAL_Led_User_Set(uint8_t state, void* reserved) {
    Set_User_LED(state);
}

/**
 * @brief  Toggle user LED
 */
void HAL_Led_User_Toggle(void* reserved) {
    Toggle_User_LED();
}

/**
 * @brief  Set mirror LED configuration
 */
led_config_t* HAL_Led_Set_Configuration(uint8_t led, led_config_t* conf, void* reserved) {
    if (led < LED_MIRROR_OFFSET) {
        return NULL;
    }
    HAL_Leds[led] = *conf;
    return &HAL_Leds[led];
}

/**
 * @brief  Get mirror LED configuration
 */
led_config_t* HAL_Led_Get_Configuration(uint8_t led, void* reserved) {
    return &HAL_Leds[led];
}

/**
 * @brief  Initialize LED
 */
void HAL_Led_Init(uint8_t led, led_config_t* conf, void* reserved) {
    if (conf) {
        HAL_Led_Set_Configuration(led, conf, NULL);
    }

    LED_Init((Led_TypeDef)led);
}

/**
 * @brief  Save mirror RGB LED to DCT
 */
static void LED_Mirror_Persist(uint8_t led, led_config_t* conf) {
    const size_t offset = DCT_LED_MIRROR_OFFSET + ((led - LED_MIRROR_OFFSET) * sizeof(led_config_t));
    led_config_t saved_config;
    dct_read_app_data_copy(offset, &saved_config, sizeof(saved_config));

    if (conf) {
        if (saved_config.version == 0xFF || memcmp((void*)conf, (void*)&saved_config, sizeof(led_config_t)))
        {
            dct_write_app_data((void*)conf, offset, sizeof(led_config_t));
        }
    } else {
        if (saved_config.version != 0xFF) {
            memset((void*)&saved_config, 0xff, sizeof(led_config_t));
            dct_write_app_data((void*)&saved_config, offset, sizeof(led_config_t));
        }
    }
}

/**
 * @brief  Disable mirror LED, clear it if in bootloader
 */
void HAL_Core_Led_Mirror_Pin_Disable(uint8_t led, uint8_t bootloader, void* reserved)
{
    int32_t state = HAL_disable_irq();
    led_config_t* ledc = HAL_Led_Get_Configuration(led, NULL);
    if (ledc->is_active) {
        ledc->is_active = 0;
        HAL_PWM_Reset_Pin(ledc->pin);
        HAL_Pin_Mode(ledc->pin, PIN_MODE_NONE);
    }
    HAL_enable_irq(state);

    if (bootloader) {
        LED_Mirror_Persist(led, NULL);
    }
}

/**
 * @brief  Set mirror LED, save it to DCT if in bootloader
 */
void HAL_Core_Led_Mirror_Pin(uint8_t led, pin_t pin, uint32_t flags, uint8_t bootloader, void* reserved)
{
    if (pin > TOTAL_PINS) {
        return;
    }

    NRF5x_Pin_Info* pinmap = HAL_Pin_Map();

    if (pinmap[pin].pwm_instance == PWM_INSTANCE_NONE) {
        return;
    }

    // NOTE: `flags` currently only control whether the LED state should be inverted
    // NOTE: All mirrored LEDs are currently PWM

    led_config_t conf = {
        .version = 0x01,
        .pin = pin,
        .is_inverted = flags,
        .is_active = 1
    };

    int32_t state = HAL_disable_irq();
    HAL_Led_Init(led, &conf, NULL);
    HAL_enable_irq(state);

    if (!bootloader) {
        LED_Mirror_Persist(led, NULL);
        return;
    }

    LED_Mirror_Persist(led, &conf);
}

void RGB_LED_Uninit() {
    for (int i = 0; i < LEDn; i++) {
        if (HAL_Leds[i].is_active) {
            HAL_PWM_Reset_Pin(HAL_Leds[i].pin);
        }
    }
}
