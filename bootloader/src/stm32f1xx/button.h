#ifndef __BUTTON_H
#define __BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "hw_config.h"

void BUTTON_Init_Ext();
uint8_t BUTTON_Is_Pressed(Button_TypeDef button);
uint16_t BUTTON_Pressed_Time(Button_TypeDef button);

#ifdef __cplusplus
}
#endif

#endif /* __BUTTON_H */
