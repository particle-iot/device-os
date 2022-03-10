/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "platform_config.h"
#include "rgbled_hal.h"
#include "rgbled.h"
#include "gpio_hal.h"
#include "pwm_hal.h"
#include "pinmap_impl.h"
#include "core_hal.h"

namespace {

hal_led_config_t leds[LEDn] = {
    {
        .version = LED_CONFIG_STRUCT_VERSION,
        .pin = LED_PIN_USER,
        {
            {
                .is_active = 1,
                .is_inverted = 0
            },
        },
        .padding = {0}
    },
    {
        .version = LED_CONFIG_STRUCT_VERSION,
        .pin = LED_PIN_RED,
        {
            {
                .is_active = 1,
                .is_inverted = 1
            },
        },
        .padding = {0}
    },
    {
        .version = LED_CONFIG_STRUCT_VERSION,
        .pin = LED_PIN_GREEN,
        {
            {
                .is_active = 1,
                .is_inverted = 1
            },
        },
        .padding = {0}
    },
    {
        .version = LED_CONFIG_STRUCT_VERSION,
        .pin = LED_PIN_BLUE,
        {
            {
                .is_active = 1,
                .is_inverted = 1
            },
        },
        .padding = {0}
    },
};

void setRgbValue(Led_TypeDef led, uint16_t value) {
    // mirror led configuration is changed, should reconfigure mirror LED
    if (leds[led].version != LED_CONFIG_STRUCT_VERSION) {
        return;
    }

    if (leds[led].is_active) {
        uint32_t pwm_max = hal_led_get_max_rgb_values(nullptr);
        hal_pwm_write_ext(leds[led].pin, leds[led].is_inverted ? (pwm_max - value) : value);
    }
}

void initLedPin(Led_TypeDef led) {
    // TODD:
// #if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
//     if (led >= LED_MIRROR_OFFSET) {
//         // Load configuration from DCT
//         hal_led_config_t conf;
//         const size_t offset = DCT_LED_MIRROR_OFFSET + ((led - LED_MIRROR_OFFSET) * sizeof(hal_led_config_t));
//         if (dct_read_app_data_copy(offset, &conf, sizeof(conf)) == 0 &&
//             conf.version != 0xff &&
//             conf.is_active) {
//             //int32_t state = HAL_disable_irq();
//             memcpy((void*)&leds[led], (void*)&conf, sizeof(hal_led_config_t));
//             //HAL_enable_irq(state);
//         } else {
//             return;
//         }
//     }
// #endif // MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
    if (leds[led].version != LED_CONFIG_STRUCT_VERSION) {
        return;
    }

    if (leds[led].is_inverted) {
        hal_gpio_write(leds[led].pin, 1);
    } else {
        hal_gpio_write(leds[led].pin, 0);
    }
    hal_gpio_mode(leds[led].pin, OUTPUT);
}

void rgbUninit() {
    for (int i = 0; i < LEDn; i++) {
        if (leds[i].is_active) {
            hal_pwm_reset_pin(leds[i].pin);
        }
    }
}

void ledMirrorPersist(uint8_t led, hal_led_config_t* conf) {
    // TODO
    // const size_t offset = DCT_LED_MIRROR_OFFSET + ((led - LED_MIRROR_OFFSET) * sizeof(hal_led_config_t));
    // hal_led_config_t saved_config;
    // dct_read_app_data_copy(offset, &saved_config, sizeof(saved_config));

    // if (conf) {
    //     if (saved_config.version == 0xFF || memcmp((void*)conf, (void*)&saved_config, sizeof(hal_led_config_t)))
    //     {
    //         dct_write_app_data((void*)conf, offset, sizeof(hal_led_config_t));
    //     }
    // } else {
    //     if (saved_config.version != 0xFF) {
    //         memset((void*)&saved_config, 0xff, sizeof(hal_led_config_t));
    //         dct_write_app_data((void*)&saved_config, offset, sizeof(hal_led_config_t));
    //     }
    // }
}

} // anonymous

void hal_led_init(uint8_t led, hal_led_config_t* conf, void* reserved) {
    if (conf) {
        hal_led_set_configuration(led, conf, nullptr);
    }
    initLedPin((Led_TypeDef)led);
}

void hal_led_set_rgb_values(uint16_t r, uint16_t g, uint16_t b, void* reserved) {
    setRgbValue(PARTICLE_LED_RED, r);
    setRgbValue(PARTICLE_LED_GREEN, g);
    setRgbValue(PARTICLE_LED_BLUE, b);

    setRgbValue((Led_TypeDef)(PARTICLE_LED_RED   + LED_MIRROR_OFFSET), r);
    setRgbValue((Led_TypeDef)(PARTICLE_LED_GREEN + LED_MIRROR_OFFSET), g);
    setRgbValue((Led_TypeDef)(PARTICLE_LED_BLUE  + LED_MIRROR_OFFSET), b);
}

void hal_led_get_rgb_values(uint16_t* rgb, void* reserved) {
    uint32_t pwm_max = hal_led_get_max_rgb_values(nullptr);

    rgb[0] = leds[PARTICLE_LED_RED].is_inverted ?
             (pwm_max - hal_pwm_get_analog_value(leds[PARTICLE_LED_RED].pin)) :
             hal_pwm_get_analog_value(leds[PARTICLE_LED_RED].pin);
    rgb[1] = leds[PARTICLE_LED_GREEN].is_inverted ?
             (pwm_max - hal_pwm_get_analog_value(leds[PARTICLE_LED_GREEN].pin)) :
             hal_pwm_get_analog_value(leds[PARTICLE_LED_GREEN].pin);
    rgb[2] = leds[PARTICLE_LED_BLUE].is_inverted ?
             (pwm_max - hal_pwm_get_analog_value(leds[PARTICLE_LED_BLUE].pin)) :
             hal_pwm_get_analog_value(leds[PARTICLE_LED_BLUE].pin);
}

uint32_t hal_led_get_max_rgb_values(void* reserved) {
    return (1 << hal_pwm_get_resolution(leds[PARTICLE_LED_RED].pin)) - 1;
}

void hal_led_set_user(uint8_t state, void* reserved) {
    if ((!state && leds[PARTICLE_LED_USER].is_inverted) || (state && !leds[PARTICLE_LED_USER].is_inverted)) {
        hal_gpio_write(leds[PARTICLE_LED_USER].pin, 1);
    } else {
        hal_gpio_write(leds[PARTICLE_LED_USER].pin, 0);
    }
}

void hal_led_toggle_user(void* reserved) {
    hal_gpio_write(leds[PARTICLE_LED_USER].pin, !hal_gpio_read(leds[PARTICLE_LED_USER].pin));
}

hal_led_config_t* hal_led_set_configuration(uint8_t led, hal_led_config_t* conf, void* reserved) {
    if (led < LED_MIRROR_OFFSET) {
        return nullptr;
    }
    leds[led] = *conf;
    return &leds[led];
}

hal_led_config_t* hal_led_get_configuration(uint8_t led, void* reserved) {
    return &leds[led];
}


// Declared in core_hal.h
void HAL_Core_Led_Mirror_Pin_Disable(uint8_t led, uint8_t bootloader, void* reserved) {
    // int32_t state = HAL_disable_irq();
    hal_led_config_t* ledc = hal_led_get_configuration(led, nullptr);
    if (ledc->is_active) {
        ledc->is_active = 0;
        hal_pwm_reset_pin(ledc->pin);
        hal_gpio_mode(ledc->pin, PIN_MODE_NONE);
    }
    // HAL_enable_irq(state);

    if (bootloader) {
        ledMirrorPersist(led, nullptr);
    }
}

void HAL_Core_Led_Mirror_Pin(uint8_t led, hal_pin_t pin, uint32_t flags, uint8_t bootloader, void* reserved) {
    if (pin > TOTAL_PINS) {
        return;
    }
    hal_pin_info_t* pinInfo = hal_pin_map() + pin;
    if (pinInfo->pwm_instance == PWM_INSTANCE_NONE) {
        return;
    }

    // NOTE: `flags` currently only control whether the LED state should be inverted
    // NOTE: All mirrored LEDs are currently PWM

    hal_led_config_t conf = {};
    conf.version = 0x01;
    conf.pin = pin;
    conf.is_active = 1;
    conf.is_inverted = (uint8_t)flags;

    // int32_t state = HAL_disable_irq();
    hal_led_init(led, &conf, nullptr);
    // HAL_enable_irq(state);

    if (!bootloader) {
        ledMirrorPersist(led, nullptr);
        return;
    }
    ledMirrorPersist(led, &conf);
}


// Deprecated, not exported in HAL

void LED_Init(Led_TypeDef led) {
    initLedPin(led);
}

void Set_RGB_LED_Values(uint16_t r, uint16_t g, uint16_t b) {
    hal_led_set_rgb_values(r, g, b, nullptr);
}

void Get_RGB_LED_Values(uint16_t* values) {
    hal_led_get_rgb_values(values, nullptr);
}

uint16_t Get_RGB_LED_Max_Value(void) {
    return hal_led_get_max_rgb_values(nullptr);
}

void Set_User_LED(uint8_t state) {
    hal_led_set_user(state, nullptr);
}

void Toggle_User_LED(void) {
    hal_led_toggle_user(nullptr);
}

void RGB_LED_Uninit() {
    rgbUninit();
}
