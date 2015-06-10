/**
 ******************************************************************************
 * @file    system_dynalib.h
 * @authors Matthew McGowan
 * @date    12 February 2015
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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

#ifndef SYSTEM_DYNALIB_H
#define	SYSTEM_DYNALIB_H

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "system_mode.h"
#include "system_sleep.h"
#include "system_task.h"
#include "system_update.h"
#endif

DYNALIB_BEGIN(system)
DYNALIB_FN(system, system_mode)
DYNALIB_FN(system, set_system_mode)
        
DYNALIB_FN(system, set_ymodem_serial_flash_update_handler)
DYNALIB_FN(system, system_firmwareUpdate)
DYNALIB_FN(system, system_fileTransfer)
        
DYNALIB_FN(system, system_delay_ms)
DYNALIB_FN(system, system_sleep)
DYNALIB_FN(system, system_sleep_pin)
DYNALIB_END(system)


#endif	/* SYSTEM_DYNALIB_H */

