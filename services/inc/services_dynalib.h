/**
 ******************************************************************************
 * @file    services-dynalib.h
 * @author  Matthew McGowan
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

#ifndef SERVICES_DYNALIB_H
#define	SERVICES_DYNALIB_H

#include "dynalib.h"

DYNALIB_BEGIN(services)

DYNALIB_FN(0, services, LED_SetRGBColor, void(uint32_t))
DYNALIB_FN(1, services, LED_SetSignalingColor, void(uint32_t))
DYNALIB_FN(2, services, LED_Signaling_Start, void(void))
DYNALIB_FN(3, services, LED_Signaling_Stop, void(void))
DYNALIB_FN(4, services, LED_SetBrightness, void(uint8_t))
DYNALIB_FN(5, services, LED_RGB_Get, void(uint8_t*))
DYNALIB_FN(6, services, LED_RGB_IsOverRidden, bool(void))
DYNALIB_FN(7, services, LED_On, void(Led_TypeDef))
DYNALIB_FN(8, services, LED_Off, void(Led_TypeDef))
DYNALIB_FN(9, services, LED_Toggle, void(Led_TypeDef))
DYNALIB_FN(10, services, LED_Fade, void(Led_TypeDef))
DYNALIB_FN(11, services, Get_LED_Brightness, uint8_t(void))

DYNALIB_FN(12, services, set_logger_output, void(debug_output_fn, LoggerOutputLevel)) // Deprecated
DYNALIB_FN(13, services, panic_, void(ePanicCode, void*, void(*)(uint32_t)))

DYNALIB_FN(14, services, jsmn_init, void(jsmn_parser*, void*))
DYNALIB_FN(15, services, jsmn_parse, jsmnerr_t(jsmn_parser*, const char*, size_t, jsmntok_t*, unsigned int, void*))
DYNALIB_FN(16, services, log_print_, void(int, int, const char*, const char*, const char*, ...)) // Deprecated
DYNALIB_FN(17, services, LED_RGB_SetChangeHandler, void(led_update_handler_fn, void*))
DYNALIB_FN(18, services, log_print_direct_, void(int, void*, const char*, ...)) // Deprecated
DYNALIB_FN(19, services, LED_GetColor, uint32_t(uint32_t, void*))

DYNALIB_FN(20, services, log_message, void(int, const char*, const char*, int, const char*, void*, const char*, ...))
DYNALIB_FN(21, services, log_message_v, void(int, const char*, const char*, int, const char*, void*, const char*, va_list))
DYNALIB_FN(22, services, log_write, void(int, const char*, const char*, size_t, void*))
DYNALIB_FN(23, services, log_printf, void(int, const char*, void*, const char*, ...))
DYNALIB_FN(24, services, log_printf_v, void(int, const char*, void*, const char*, va_list))
DYNALIB_FN(25, services, log_dump, void(int, const char*, const void*, size_t, int, void*))
DYNALIB_FN(26, services, log_enabled, int(int, const char*, void*))
DYNALIB_FN(27, services, log_level_name, const char*(int, void*))
DYNALIB_FN(28, services, log_set_callbacks, void(log_message_callback_type, log_write_callback_type, log_enabled_callback_type, void*))

DYNALIB_END(services)

#endif	/* SERVICES_DYNALIB_H */

