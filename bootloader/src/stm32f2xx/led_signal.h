#ifndef LED_SIGNAL_H
#define LED_SIGNAL_H

#include <stdint.h>

// Loads some of the colors used for LED signaling from DCT
void get_led_theme_colors(uint32_t* firmware_update, uint32_t* safe_mode, uint32_t* dfu_mode);

#endif // LED_SIGNAL_H
