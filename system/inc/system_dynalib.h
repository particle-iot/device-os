/**
 ******************************************************************************
 * @file    system_dynalib.h
 * @authors Matthew McGowan
 * @date    12 February 2015
 ******************************************************************************
  Copyright (c) 2015 Spark Labs, Inc.  All rights reserved.

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

DYNALIB_BEGIN(system)
DYNALIB_FN(system, spark_variable)
DYNALIB_FN(system, spark_function)
DYNALIB_FN(system, spark_process)
DYNALIB_FN(system, spark_connect)
DYNALIB_FN(system, spark_disconnect)
DYNALIB_FN(system, spark_connected)
DYNALIB_FN(system, spark_protocol_instance)
DYNALIB_FN(system, spark_deviceID)
DYNALIB_FN(system, system_mode)
DYNALIB_FN(system, set_system_mode)
        
DYNALIB_FN(system, network_config)
DYNALIB_FN(system, network_connect)
DYNALIB_FN(system, network_connecting)
DYNALIB_FN(system, network_disconnect)
DYNALIB_FN(system, network_ready)
DYNALIB_FN(system, network_on)
DYNALIB_FN(system, network_off)
DYNALIB_FN(system, network_listen)
DYNALIB_FN(system, network_listening)
DYNALIB_FN(system, network_has_credentials)
DYNALIB_FN(system, network_set_credentials)
DYNALIB_FN(system, network_clear_credentials)
DYNALIB_FN(system, set_ymodem_serial_flash_update_handler)
DYNALIB_FN(system, system_serialSaveFile)
DYNALIB_FN(system, system_serialFirmwareUpdate)
DYNALIB_END(system)


#endif	/* SYSTEM_DYNALIB_H */

