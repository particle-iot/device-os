#ifndef _BUTTON_HAL_IMPL_H
#define _BUTTON_HAL_IMPL_H

#include "nrfx_types.h"
#include "nrf_gpio.h"
#include "nrf_gpiote.h"

#ifdef __cplusplus
extern "C" {
#endif

//Push Buttons
#define BUTTON1_GPIO_PIN                    11
#define BUTTON1_GPIO_MODE                   NRF_GPIO_PIN_DIR_INPUT
#define BUTTON1_GPIO_PUPD                   NRF_GPIO_PIN_PULLUP
#define BUTTON1_PRESSED                     0x00
#define BUTTON1_GPIOTE_EVENT_IN             NRF_GPIOTE_EVENTS_IN_0
#define BUTTON1_GPIOTE_EVENT_CHANNEL        0
#define BUTTON1_GPIOTE_INT_MASK             NRF_GPIOTE_INT_IN0_MASK
#define BUTTON1_GPIOTE_IRQn                 GPIOTE_IRQn
#define BUTTON1_GPIOTE_IRQ_HANDLER          GPIOTE_IRQHandler
#define BUTTON1_GPIOTE_IRQ_PRIORITY         7
#define BUTTON1_GPIOTE_IRQ_INDEX            22
#define BUTTON1_GPIOTE_TRIGGER              NRF_GPIOTE_POLARITY_HITOLO
#define BUTTON1_MIRROR_SUPPORTED            0

typedef struct {
    uint16_t              pin;
    nrf_gpio_pin_dir_t    mode;
    nrf_gpio_pin_pull_t   pupd;
    volatile uint8_t      active;
    volatile uint16_t     debounce_time;
    uint16_t              event_in;
    uint16_t              event_channel;
    uint16_t              int_mask;
    uint16_t              interrupt_mode;
    uint16_t              nvic_irqn;
    uint16_t              nvic_irq_prio;
    uint8_t               padding[12];
} button_config_t;

#ifdef __cplusplus
}
#endif

#endif
