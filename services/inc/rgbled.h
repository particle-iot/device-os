#ifndef RGBLED_H
#define	RGBLED_H

// NOTE: Do not use this API for system RGB LED signaling unless a low-level control over
// the LED is required. Consider using LEDStatus class instead

#include <stdbool.h>
#include <stdint.h>
#include "platform_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t version;
    void (*Led_Rgb_Set_Values)(uint16_t r, uint16_t g, uint16_t b, void* reserved);
    void (*Led_Rgb_Get_Values)(uint16_t* rgb, void* reserved);
    uint32_t (*Led_Rgb_Get_Max_Value)(void* reserved);
    void (*Led_User_Set)(uint8_t state, void* reserved);
    void (*Led_User_Toggle)(void* reserved);
} LedCallbacks;

#define DEFAULT_LED_RGB_BRIGHTNESS (96)

typedef uint8_t Led_TypeDef;

#define LED_MIRROR_OFFSET          (4)

#define PARTICLE_LED1               (0)
#define PARTICLE_LED2               (1)
#define PARTICLE_LED3               (2)
#define PARTICLE_LED4               (3)
#define PARTICLE_LED3_LED4_LED2     (231)
#define PARTICLE_LED1_MIRROR        (PARTICLE_LED1 + LED_MIRROR_OFFSET)
#define PARTICLE_LED2_MIRROR        (PARTICLE_LED2 + LED_MIRROR_OFFSET)
#define PARTICLE_LED3_MIRROR        (PARTICLE_LED3 + LED_MIRROR_OFFSET)
#define PARTICLE_LED4_MIRROR        (PARTICLE_LED4 + LED_MIRROR_OFFSET)

//Extended LED Types
#define PARTICLE_LED_RGB            (PARTICLE_LED3_LED4_LED2)
#define PARTICLE_LED_USER           (PARTICLE_LED1)

//RGB Basic Colors
#define RGB_COLOR_RED     0xFF0000
#define RGB_COLOR_GREEN   0x00FF00
#define RGB_COLOR_BLUE    0x0000FF
#define RGB_COLOR_YELLOW  0xFFFF00
#define RGB_COLOR_CYAN    0x00FFFF
#define RGB_COLOR_MAGENTA 0xFF00FF
#define RGB_COLOR_WHITE   0xFFFFFF
#define RGB_COLOR_ORANGE  0xFF6000
#define RGB_COLOR_GREY    0x1F1F1F
#define RGB_COLOR_GRAY    0x1F1F1F

extern volatile uint8_t led_rgb_brightness;

void LED_SetRGBColor(uint32_t RGB_Color);
void LED_SetSignalingColor(uint32_t RGB_Color);
void LED_Signaling_Start(void);
void LED_Signaling_Stop(void);
void LED_SetBrightness(uint8_t brightness); /* 0 = off, 255 = full brightness */
void LED_RGB_Get(uint8_t* rgb);
void LED_Init(Led_TypeDef Led);
void LED_On(Led_TypeDef Led);
void LED_Off(Led_TypeDef Led);
void LED_Toggle(Led_TypeDef Led);
void LED_Fade(Led_TypeDef Led);
uint32_t LED_GetColor(uint32_t index, void* reserved);


uint8_t Get_LED_Brightness();
// Hardware interface

/**
 * Directly set the color of the RGB led.
 */
void Set_RGB_LED(uint16_t* data);
bool LED_RGB_IsOverRidden(void);

/**
 * The function that handles notifications of changes to the RGB led.
 */
typedef void (*led_update_handler_fn)(void* data, uint8_t r, uint8_t g, uint8_t b, void* reserved);

extern led_update_handler_fn led_update_handler;
extern void* led_update_handler_data;

void LED_RGB_SetChangeHandler(led_update_handler_fn fn, void* data);


void LED_SetCallbacks(LedCallbacks cb, void* reserved);

extern LedCallbacks LED_Callbacks;

#ifdef __cplusplus
}
#endif

#endif	/* RGBLED_H */
