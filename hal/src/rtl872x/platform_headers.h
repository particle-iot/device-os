#pragma once

/**
 * This header file can be used to control access to low-level platform
 * features in user code. Normally user code should access the platform via
 * the HAL.
 */

#define retained __attribute__((section(".retained_user")))
#define retained_system __attribute__((section(".retained_system")))

// Expose CMSIS/NVIC/IRQ/etc-related header files to the application
#include "interrupts_irq.h"
// Expose some of the peripheral registers
#include "hal_platform_rtl.h"
// GPIO/pinmux/fast-pin related stuff
#include "rtl8721d_pinmux_defines.h"
