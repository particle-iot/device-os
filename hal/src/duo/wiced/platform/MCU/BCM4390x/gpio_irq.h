/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include <stdint.h>
#include "wwd_constants.h"
#include "wiced_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef int                      gpio_port_t;
typedef uint8_t                  gpio_pin_number_t;
typedef wiced_gpio_irq_trigger_t gpio_irq_trigger_t;
typedef wiced_gpio_irq_handler_t gpio_irq_handler_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/* Global GPIO interrupt handler */
//void gpio_irq( void );

/* GPIO Interrupt API */
wiced_result_t gpio_irq_enable ( gpio_port_t* gpio_port, gpio_pin_number_t gpio_pin_number, gpio_irq_trigger_t trigger, gpio_irq_handler_t handler, void* arg );
wiced_result_t gpio_irq_disable( gpio_port_t* gpio_port, gpio_pin_number_t gpio_pin_number );

#ifdef __cplusplus
} /*extern "C" */
#endif
