#ifndef _BUTTON_HAL_IMPL_H
#define _BUTTON_HAL_IMPL_H

#include "nrfx_types.h"
#include "nrf_gpio.h"
#include "nrf_gpiote.h"

#ifdef __cplusplus
extern "C" {
#endif

//Push Buttons
#define BUTTON1_GPIO_PIN                    20
#define BUTTON1_GPIOTE_INTERRUPT_MODE       FALLING
#define BUTTON1_PRESSED                     0x00
#define BUTTON1_MIRROR_SUPPORTED            0

typedef struct {
    uint16_t              pin;
    uint8_t               interrupt_mode;
    volatile uint8_t      active;
    volatile uint16_t     debounce_time;
    uint8_t               padding[26];
} button_config_t;

#ifdef __cplusplus
}
#endif

#endif
