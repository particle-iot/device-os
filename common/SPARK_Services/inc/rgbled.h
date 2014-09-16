
#ifndef RGBLED_H
#define	RGBLED_H

typedef enum
{
	LED1 = 0, LED2 = 1, LED3 = 2, LED4 = 3, LED3_LED4_LED2 = 231
} Led_TypeDef;


//Extended LED Types
#define LED_RGB				LED3_LED4_LED2
#define LED_USER			LED1

//RGB Basic Colors
#define RGB_COLOR_RED		0xFF0000
#define RGB_COLOR_GREEN		0x00FF00
#define RGB_COLOR_BLUE		0x0000FF
#define RGB_COLOR_YELLOW	0xFFFF00
#define RGB_COLOR_CYAN		0x00FFFF
#define RGB_COLOR_MAGENTA	0xFF00FF
#define RGB_COLOR_WHITE		0xFFFFFF
#define RGB_COLOR_ORANGE  0xFF6000


void LED_SetRGBColor(uint32_t RGB_Color);
void LED_SetSignalingColor(uint32_t RGB_Color);
void LED_Signaling_Start(void);
void LED_Signaling_Stop(void);
void LED_Signaling_Override(void) __attribute__ ((weak));
void LED_SetBrightness(uint8_t brightness); /* 0 = off, 255 = full brightness */
void LED_RGB_Get(uint8_t* rgb);
void LED_Init(Led_TypeDef Led);
void LED_On(Led_TypeDef Led);
void LED_Off(Led_TypeDef Led);
void LED_Toggle(Led_TypeDef Led);
void LED_Fade(Led_TypeDef Led);

uint8_t Get_LED_Brightness();
// Hardware interface

/**
 * Directly set the color of the RGB led.
 */
void Set_RGB_LED(uint16_t* data);
extern void Set_RGB_LED_Values(uint16_t r, uint16_t g, uint16_t b);

extern void Get_RGB_LED_Values(uint16_t* rgb);

extern void Set_User_LED(uint8_t state);
extern void Toggle_User_LED();

extern uint16_t Get_RGB_LED_Max_Value();


#endif	/* RGBLED_H */

