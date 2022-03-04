#ifndef __BUTTON_H
#define __BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "hw_config.h"

void hal_button_init_ext();

uint8_t hal_button_is_pressed(hal_button_t button);
uint16_t hal_button_get_pressed_time(hal_button_t button);


void hal_button_irq_handler(uint16_t exti);
void hal_button_check_irq(uint16_t button, uint16_t exti);
void hal_button_check_state(uint16_t button, uint8_t pressed);
int hal_button_debounce();

#ifdef __cplusplus
}
#endif

#endif /* __BUTTON_H */
