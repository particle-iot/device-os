

#include "rgbled_hal.h"

void Set_RGB_LED_Values(uint16_t r, uint16_t g, uint16_t b)
{

}

void Get_RGB_LED_Values(uint16_t* rgb)
{

}

void Set_User_LED(uint8_t state)
{

}

void Toggle_User_LED(void)
{

}

uint16_t Get_RGB_LED_Max_Value(void)
{
    return 65535;
}

void hal_led_set_rgb_values(uint16_t r, uint16_t g, uint16_t b, void* reserved)
{
}

void hal_led_get_rgb_values(uint16_t* rgb, void* reserved)
{
}

uint32_t hal_led_get_max_rgb_values(void* reserved)
{
    return 65535;
}

void hal_led_set_user(uint8_t state, void* reserved)
{
}

void hal_led_toggle_user(void* reserved)
{
}

hal_led_config_t* hal_led_set_configuration(uint8_t led, hal_led_config_t* conf, void* reserved)
{
    return nullptr;
}

hal_led_config_t* hal_led_get_configuration(uint8_t led, void* reserved)
{
    return nullptr;
}

void hal_led_init(uint8_t led, hal_led_config_t* conf, void* reserved)
{
}
