#include "rgbled_hal.h"
#include "rgbled.h"
#include "module_info.h"
#include "platform_config.h"
#include "nrf_pwm.h"
#include "nrf_gpio.h"


// LEDs
#define LEDn                                4
#define LED_MIRROR_SUPPORTED                0
#define LED_BLUE                            LED2                    //BLUE Led
#define LED_RED                             LED3                    //RED Led
#define LED_GREEN                           LED4                    //GREEN Led

// FIXME: It should fetch the pin number and PWM configurations from pinmap_hal.c
#if PLATFORM_ID == PLATFORM_ARGON_SOM || PLATFORM_ID == PLATFORM_XENON_SOM
#define LED1_GPIO_PIN                       44                      //User Led
#define LED2_GPIO_PIN                       45                      //BLUE Led
#define LED3_GPIO_PIN                       47                      //RED Led
#define LED4_GPIO_PIN                       46                      //GREEN Led
#else
#define LED1_GPIO_PIN                       44                      //User Led
#define LED2_GPIO_PIN                       15                      //BLUE Led
#define LED3_GPIO_PIN                       13                      //RED Led
#define LED4_GPIO_PIN                       14                      //GREEN Led
#endif


led_config_t HAL_Leds[] = {
    {
        .version = 0x00,
        .pin = LED1_GPIO_PIN,
        .is_inverted = 0
    },
    {
        .version = 0x00,
        .pin = LED2_GPIO_PIN,
        .is_inverted = 1
    },
    {
        .version = 0x00,
        .pin = LED3_GPIO_PIN,
        .is_inverted = 1
    },
    {
        .version = 0x00,
        .pin = LED4_GPIO_PIN,
        .is_inverted = 1
    },
};

static nrf_pwm_values_wave_form_t rgb_wave_form_values;


static void RGB_PWM_Config(void)
{
    uint32_t output_pins[NRF_PWM_CHANNEL_COUNT];
    static nrf_pwm_sequence_t rgb_seq;

    output_pins[0] = HAL_Leds[LED_RED].pin;
    output_pins[1] = HAL_Leds[LED_GREEN].pin;
    output_pins[2] = HAL_Leds[LED_BLUE].pin;
    output_pins[3] = NRF_PWM_PIN_NOT_CONNECTED;

    nrf_pwm_pins_set(NRF_PWM0, output_pins);

    // Base clock: 500KHz, Count mode: up counter, COUNTERTOP: 0(since we use the wave form load mode).
    nrf_pwm_configure(NRF_PWM0, NRF_PWM_CLK_500kHz, NRF_PWM_MODE_UP, 0);

    // Load mode: wave form, Refresh mode: auto
    nrf_pwm_decoder_set(NRF_PWM0, NRF_PWM_LOAD_WAVE_FORM, NRF_PWM_STEP_AUTO);

    // Configure the RGB PWM sequence, use sequence0 only
    rgb_wave_form_values.counter_top = 255;
    rgb_seq.values.p_wave_form       = &rgb_wave_form_values;
    rgb_seq.length                   = NRF_PWM_VALUES_LENGTH(rgb_wave_form_values);
    rgb_seq.repeats                  = 0;
    rgb_seq.end_delay                = 0;
    nrf_pwm_sequence_set(NRF_PWM0, 0, &rgb_seq);

    nrf_pwm_enable(NRF_PWM0);
}

void Set_RGB_LED_Values(uint16_t r, uint16_t g, uint16_t b)
{
    // TBD: Change the polarity for inverted RGB connection
    rgb_wave_form_values.channel_0 = r;
    rgb_wave_form_values.channel_1 = g;
    rgb_wave_form_values.channel_2 = b;

    // Starts the PWM generation
    nrf_pwm_task_trigger(NRF_PWM0, NRF_PWM_TASK_SEQSTART0);
}

void Get_RGB_LED_Values(uint16_t* values)
{
    values[0] = rgb_wave_form_values.channel_0;
    values[1] = rgb_wave_form_values.channel_1;
    values[2] = rgb_wave_form_values.channel_2;
}

uint16_t Get_RGB_LED_Max_Value(void)
{
    return rgb_wave_form_values.counter_top;
}

void Set_User_LED(uint8_t state)
{
    if ((!state && HAL_Leds[LED_USER].is_inverted) || \
        (state && !HAL_Leds[LED_USER].is_inverted))
    {
        nrf_gpio_pin_set(HAL_Leds[LED_USER].pin);
    }
    else
    {
        nrf_gpio_pin_clear(HAL_Leds[LED_USER].pin);
    }
}

void Toggle_User_LED(void)
{
    nrf_gpio_pin_toggle(HAL_Leds[LED_USER].pin);
}

/**
 * @brief  Configures LED GPIO.
 * @param  Led: Specifies the Led to be configured.
 *   This parameter can be one of following parameters:
 *     @arg LED1, LED2, LED3, LED4
 * @retval None
 */
void LED_Init(Led_TypeDef Led)
{
    nrf_gpio_cfg(
        HAL_Leds[Led].pin,
        NRF_GPIO_PIN_DIR_OUTPUT,
        NRF_GPIO_PIN_INPUT_DISCONNECT,
        NRF_GPIO_PIN_NOPULL,
        NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_NOSENSE);

    if (HAL_Leds[Led].is_inverted)
    {
        nrf_gpio_pin_set(HAL_Leds[Led].pin);
    }
    else
    {
        nrf_gpio_pin_clear(HAL_Leds[Led].pin);
    }

    RGB_PWM_Config();
}

void HAL_Led_Rgb_Set_Values(uint16_t r, uint16_t g, uint16_t b, void* reserved)
{
    Set_RGB_LED_Values(r, g, b);
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
    Set_User_LED(state);
}

void HAL_Led_User_Toggle(void* reserved)
{
    Toggle_User_LED();
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
        return (led_config_t*)&HAL_Leds[led];
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
}
