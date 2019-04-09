/**
 ******************************************************************************
 * @file    system_dynalib_cloud.h
 * @authors mat
 * @date    04 March 2015
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

#ifndef SYSTEM_DYNALIB_CLOUD_H
#define	SYSTEM_DYNALIB_CLOUD_H

#include "dynalib.h"
#include "system_tick_hal.h"

#ifdef DYNALIB_EXPORT
#include "system_cloud.h"
#endif


DYNALIB_BEGIN(system_cloud)

DYNALIB_FN(0, system_cloud, spark_variable, bool(const char*, const void*, Spark_Data_TypeDef, spark_variable_t*))
DYNALIB_FN(1, system_cloud, spark_function, bool(const char*, p_user_function_int_str_t, void*))
DYNALIB_FN(2, system_cloud, spark_process, void(void))
DYNALIB_FN(3, system_cloud, spark_cloud_flag_connect, void(void))
DYNALIB_FN(4, system_cloud, spark_cloud_flag_disconnect, void(void))
DYNALIB_FN(5, system_cloud, spark_cloud_flag_connected, bool(void))
DYNALIB_FN(6, system_cloud, system_cloud_protocol_instance, ProtocolFacade*(void))
DYNALIB_FN(7, system_cloud, spark_deviceID, String(void))
DYNALIB_FN(8, system_cloud, spark_send_event, bool(const char*, const char*, int, uint32_t, void*))
DYNALIB_FN(9, system_cloud, spark_subscribe, bool(const char*, EventHandler, void*, Spark_Subscription_Scope_TypeDef, const char*, void*))
DYNALIB_FN(10, system_cloud, spark_unsubscribe, void(void*))
DYNALIB_FN(11, system_cloud, spark_sync_time, bool(void*))
DYNALIB_FN(12, system_cloud, spark_sync_time_pending, bool(void*))
DYNALIB_FN(13, system_cloud, spark_sync_time_last, system_tick_t(time_t*, void*))
DYNALIB_FN(14, system_cloud, spark_set_connection_property, int(unsigned, unsigned, particle::protocol::connection_properties_t*, void*))
DYNALIB_FN(15, system_cloud, spark_set_random_seed_from_cloud_handler, int(void (*handler)(unsigned int), void*))
DYNALIB_FN(16, system_cloud, spark_publish_vitals, int(system_tick_t, void*))

DYNALIB_END(system_cloud)

#endif	/* SYSTEM_DYNALIB_CLOUD_H */
