

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

void HAL_Led_Rgb_Set_Values(uint16_t r, uint16_t g, uint16_t b, void* reserved)
{
}

void HAL_Led_Rgb_Get_Values(uint16_t* rgb, void* reserved)
{
}

uint32_t HAL_Led_Rgb_Get_Max_Value(void* reserved)
{
    return 65535;
}

void HAL_Led_User_Set(uint8_t state, void* reserved)
{
}

void HAL_Led_User_Toggle(void* reserved)
{
}

led_config_t* HAL_Led_Set_Configuration(uint8_t led, led_config_t* conf, void* reserved)
{
    return nullptr;
}

led_config_t* HAL_Led_Get_Configuration(uint8_t led, void* reserved)
{
    return nullptr;
}

void HAL_Led_Init(uint8_t led, led_config_t* conf, void* reserved)
{
}
