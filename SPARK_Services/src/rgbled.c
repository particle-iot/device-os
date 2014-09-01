
#include <stdint.h>
#include "rgbled.h"

uint8_t LED_RGB_OVERRIDE = 0;
uint8_t LED_RGB_BRIGHTNESS = 96;
uint32_t lastSignalColor = 0;
uint32_t lastRGBColor = 0;

/* Led Fading. */
#define NUM_LED_FADE_STEPS 100 /* Called at 100Hz, fade over 1 second. */
static uint8_t led_fade_step = NUM_LED_FADE_STEPS - 1;
static uint8_t led_fade_direction = -1; /* 1 = rising, -1 = falling. */

uint16_t ccr_scale(uint8_t color) {
    return (uint16_t)((((uint32_t)(color)) * LED_RGB_BRIGHTNESS * Get_RGB_LED_Max_Value()) >> 16);
}

void Set_CCR_Color(uint32_t RGB_Color, uint16_t* ccr) {
    ccr[0] = ccr_scale((RGB_Color>>16) & 0xFF);
    ccr[1] = ccr_scale((RGB_Color>>8) & 0xFF);
    ccr[2] = ccr_scale(RGB_Color & 0xFF);
}

void LED_SetRGBColor(uint32_t RGB_Color)
{
    lastRGBColor = RGB_Color;
}

void LED_SetSignalingColor(uint32_t RGB_Color)
{
    lastSignalColor = RGB_Color;
}

void LED_Signaling_Start(void)
{
    LED_RGB_OVERRIDE = 1;

    LED_Off(LED_RGB);
}

void LED_Signaling_Stop(void)
{
    LED_RGB_OVERRIDE = 0;

    LED_On(LED_RGB);
}

void LED_SetBrightness(uint8_t brightness)
{
    LED_RGB_BRIGHTNESS = brightness;
}

uint8_t Get_LED_Brightness() 
{
    return LED_RGB_BRIGHTNESS;
}

/**
 * Sets the color on the RGB led. The color is adjusted for brightness.
 * @param color
 */

void Set_RGB_LED_Color(uint32_t color) {
    uint16_t ccr[3];
    Set_CCR_Color(color, ccr);
    Set_RGB_LED(ccr);
}

uint16_t scale_fade(uint8_t step, uint16_t value) {
    return (uint16_t)((((uint32_t) value) * step) / (NUM_LED_FADE_STEPS - 1));
}

void Set_RGB_LED_Scale(uint8_t step, uint32_t color) {
    int i;
    uint16_t ccr[3];
    Set_CCR_Color(color, ccr);
    for (i=0; i<3; i++)
        ccr[i] = scale_fade(step, ccr[i]);
    Set_RGB_LED(ccr);
}


/**
  * @brief  Turns selected LED On.
  * @param  Led: Specifies the Led to be set on.
  *   This parameter can be one of following parameters:
  *     @arg LED1, LED2, LED_USER, LED_RGB
  * @retval None
  */
void LED_On(Led_TypeDef Led)
{
    switch(Led)
    {
    case LED_USER:
        Set_User_LED(1);
        break;

    case LED_RGB:	//LED_SetRGBColor() should be called first for this Case        
        Set_RGB_LED_Color(LED_RGB_OVERRIDE ? lastSignalColor : lastRGBColor);
        led_fade_step = NUM_LED_FADE_STEPS - 1;
        led_fade_direction = -1; /* next fade is falling */
        break;
    default:
        break;
    }
}

/**
  * @brief  Turns selected LED Off.
  * @param  Led: Specifies the Led to be set off.
  *   This parameter can be one of following parameters:
  *     @arg LED1, LED2, LED_USER, LED_RGB
  * @retval None
  */
void LED_Off(Led_TypeDef Led)
{
    switch(Led)
    {
    case LED_USER:
        Set_User_LED(0);
        break;

    case LED_RGB:
        Set_RGB_LED_Values(0,0,0);
        led_fade_step = 0;
        led_fade_direction = 1; /* next fade is rising. */
        break;
    default:
        break;
    }
}



/**
  * @brief  Toggles the selected LED.
  * @param  Led: Specifies the Led to be toggled.
  *   This parameter can be one of following parameters:
  *     @arg LED1, LED2, LED_USER, LED_RGB
  * @retval None
  */
void LED_Toggle(Led_TypeDef Led)
{
    uint32_t color;
    uint16_t rgb[3];
    switch(Led)
    {
    case LED_USER:
        Toggle_User_LED();
        break;
    default:
        break;

    case LED_RGB://LED_SetRGBColor() and LED_On() should be called first for this Case        
        
        color = LED_RGB_OVERRIDE ? lastSignalColor : lastRGBColor;
        Get_RGB_LED_Values(rgb);
        if (rgb[0] | rgb[1] | rgb[2])
            Set_RGB_LED_Values(0,0,0);
        else
            Set_RGB_LED_Color(color);        
        break;
    }
}


/**
  * @brief  Fades selected LED.
  * @param  Led: Specifies the Led to be set on.
  *   This parameter can be one of following parameters:
  *     @arg LED1, LED2, LED_RGB
  * @retval None
  */
void LED_Fade(Led_TypeDef Led)
{
  /* Update position in fade. */
    if (led_fade_step == 0)
        led_fade_direction = 1; /* Switch to fade growing. */
    else if (led_fade_step == NUM_LED_FADE_STEPS - 1)
        led_fade_direction = -1; /* Switch to fade falling. */

    led_fade_step += led_fade_direction;

    if(Led == LED_RGB)
    {
        Set_RGB_LED_Scale(led_fade_step, LED_RGB_OVERRIDE ? lastSignalColor : lastRGBColor);
    }
}


void LED_RGB_Get(uint8_t* rgb) {
    int i=0;
    uint16_t values[3];
    Get_RGB_LED_Values(values);
    for (i=0; i<3; i++) {
        rgb[i] = (uint8_t)(((uint32_t)(values[i])<<8)/(Get_RGB_LED_Max_Value()));
    }
}

void Set_RGB_LED(uint16_t* data) {
    Set_RGB_LED_Values(data[0], data[1], data[2]);
}
