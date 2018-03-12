/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef GPIO_TRACE_H_
#define GPIO_TRACE_H_

#include "../trace.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef TRACE_ENABLE_GPIO
#define TRACE_T_GPIO                                                \
    {                                                               \
            (char*) "gpio",                                         \
            gpio_trace_start_trace,                                 \
            gpio_trace_stop_trace,                                  \
            NULL,                                                   \
            NULL,                                                   \
            gpio_trace_task_hook,                                   \
            gpio_trace_tick_hook,                                   \
            (trace_process_t []) {                                  \
                    { NULL, NULL, NULL }                            \
            }                                                       \
    },
#else
#define TRACE_T_GPIO
#endif /* TRACE_ENABLE_GPIO */

/******************************************************
 *        Hook functions
 ******************************************************/
void gpio_trace_task_hook( TRACE_TASK_HOOK_SIGNATURE );
void gpio_trace_tick_hook( TRACE_TICK_HOOK_SIGNATURE );


/******************************************************
 *        API functions to start/pause/end tracing
 ******************************************************/
void gpio_trace_start_trace( trace_flush_function_t flush_f );
void gpio_trace_stop_trace( void );
void gpio_trace_cleanup_trace( void );

/******************************************************
 *        Configuration
 ******************************************************/
/**
 * The TCB number is encoded in the output.
 */
#define TRACE_OUTPUT_TYPE_ENCODED   (1)

/**
 * Each task occupies a separate GPIO bit. This can makes results easier to see
 * immediately, but drastically reduces the number of tasks that can be output.
 */
#define TRACE_OUTPUT_TYPE_ONEBIT    (2)

#define TRACE_OUTPUT_TYPE           TRACE_OUTPUT_TYPE_ENCODED

/******************************************************
 *        Platform specific
 ******************************************************/
struct breakout_pins_setting_t;
/** Get the pin number of a specific breakout pin. */
inline uint16_t breakout_pins_get_pin_number( struct breakout_pins_setting_t * );
/** Initialize a specific breakout pin. Returns 1 if the specified pin was able to be used for output, otherwise 0. */
inline int breakout_pins_gpio_init( struct breakout_pins_setting_t * );
/** Sets the specified breakout pin. */
inline void breakout_pins_gpio_setbits( struct breakout_pins_setting_t * );
/** Resets the specified breakout pin. */
inline void breakout_pins_gpio_resetbits( struct breakout_pins_setting_t * );

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* GPIO_TRACE_H_ */
