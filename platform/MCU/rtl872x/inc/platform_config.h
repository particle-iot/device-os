/**
  Copyright (c) 2013-2018 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PLATFORM_CONFIG_H
#define __PLATFORM_CONFIG_H

#define NO_STATIC_ASSERT
#include "platforms.h"
#include "logging.h"

#ifndef PLATFORM_ID
#error "PLATFORM_ID not defined"
#endif

#define UI_TIMER_FREQUENCY                  100    /* 100Hz -> 10ms */
#define BUTTON_DEBOUNCE_INTERVAL            (1000 / UI_TIMER_FREQUENCY)

#define USE_SERIAL_FLASH

#ifdef USE_SERIAL_FLASH
#define sFLASH_PAGESIZE                     0x1000 /* 4096 bytes sector size that needs to be erased */
#define sFLASH_PAGECOUNT                    2048   /* 8MByte storage */
#define sFLASH_FILESYSTEM_PAGE_COUNT        (512) /* 2MB */
#define sFLASH_FILESYSTEM_FIRST_PAGE        (sFLASH_PAGECOUNT - sFLASH_FILESYSTEM_PAGE_COUNT)
#endif

#define FLASH_UPDATE_MODULES

#define PREPSTRING2(x) #x
#define PREPSTRING(x) PREPSTRING2(x)

#define SYSTICK_IRQ_PRIORITY                7      //CORTEX_M33 Systick Interrupt

// Currently works with platforms P2, TrackerM and MSoM
#define INTERNAL_FLASH_SIZE             (0x800000)

//Push Buttons, use interrupt HAL
#define BUTTON1_PIN                                 BTN
#define BUTTON1_PIN_MODE                            INPUT_PULLUP
#define BUTTON1_INTERRUPT_MODE                      FALLING
#define BUTTON1_PRESSED                             0x00
#define BUTTON1_MIRROR_SUPPORTED                    1
#define BUTTON1_MIRROR_PRESSED                      0x00
#define BUTTON1_MIRROR_PIN                          PIN_INVALID
#define BUTTON1_MIRROR_PIN_MODE                     INPUT_PULLUP
#define BUTTON1_MIRROR_INTERRUPT_MODE               FALLING

//RGB LEDs
#define LED_MIRROR_SUPPORTED                        1
#define LEDn                                        (4 * (LED_MIRROR_SUPPORTED + 1))
#define PARTICLE_LED_RED                            PARTICLE_LED2  // RED Led
#define PARTICLE_LED_GREEN                          PARTICLE_LED3  // GREEN Led
#define PARTICLE_LED_BLUE                           PARTICLE_LED4  // BLUE Led
#define LED_PIN_USER                                D7
#define LED_PIN_RED                                 RGBR
#define LED_PIN_GREEN                               RGBG
#define LED_PIN_BLUE                                RGBB

/* Exported functions ------------------------------------------------------- */

#endif /* __PLATFORM_CONFIG_H */
