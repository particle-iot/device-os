/**
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

#ifndef SYSTEM_MODE_H
#define	SYSTEM_MODE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  DEFAULT=0, AUTOMATIC = 1, SEMI_AUTOMATIC = 2, MANUAL = 3, SAFE_MODE=4
} System_Mode_TypeDef;

void set_system_mode(System_Mode_TypeDef mode);
System_Mode_TypeDef system_mode();

namespace spark {
    namespace feature {
        enum State {
            DISABLED =0,
            ENABLED =1,
        };
    }

}

void system_thread_set_state(spark::feature::State feature, void* reserved);
spark::feature::State system_thread_get_state(void*);
uint16_t system_button_pushed_duration(uint8_t button, void* reserved);

/**
 * System reset mode.
 */
typedef enum system_reset_mode {
    SYSTEM_RESET_MODE_INVALID_ = 0, ///< Invalid reset mode.
    SYSTEM_RESET_MODE_NORMAL = 1, ///< Normal system reset.
    SYSTEM_RESET_MODE_DFU = 2, ///< Reset into DFU mode.
    SYSTEM_RESET_MODE_SAFE = 3, ///< Reset into safe mode.
    SYSTEM_RESET_MODE_FACTORY = 4 ///< Factory reset.
} system_reset_mode;

/**
 * System reset flags.
 */
typedef enum system_reset_flag {
    SYSTEM_RESET_FLAG_NO_WAIT = 0x01, ///< Reset immediately.
    SYSTEM_RESET_FLAG_PERSIST_DFU = 0x02 ///< Persistent DFU mode (see `System.dfu()`).
} system_reset_flag;

/**
 * Reset the system.
 *
 * @param mode Reset mode (a value defined by the `system_reset_mode` enum).
 * @param reason Reset reason (a value defined by the `system_reset_reason` enum).
 * @param value Additional mode-specific parameter.
 * @param flags System reset flags (a combination of flags defined by the `system_reset_flag` enum).
 * @param reserved This argument should be set to NULL.
 * @return 0 on success or a negative result code in case of an error.
 */
int system_reset(unsigned mode, unsigned reason, unsigned value, unsigned flags, void* reserved);

#ifdef __cplusplus
}
#endif

#endif	/* SYSTEM_MODE_H */

