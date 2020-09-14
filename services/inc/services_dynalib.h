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

#ifdef DYNALIB_EXPORT
#include "nanopb_misc.h"
#include <stdint.h>
#ifdef PB_WITHOUT_64BIT
#define pb_int64_t int32_t
#define pb_uint64_t uint32_t
#else
#define pb_int64_t int64_t
#define pb_uint64_t uint64_t
#endif
#endif // defined(DYNALIB_EXPORT)

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

DYNALIB_FN(20, services, log_message, void(int, const char*, LogAttributes*, void*, const char*, ...))
DYNALIB_FN(21, services, log_message_v, void(int, const char*, LogAttributes*, void*, const char*, va_list))
DYNALIB_FN(22, services, log_write, void(int, const char*, const char*, size_t, void*))
DYNALIB_FN(23, services, log_printf, void(int, const char*, void*, const char*, ...))
DYNALIB_FN(24, services, log_printf_v, void(int, const char*, void*, const char*, va_list))
DYNALIB_FN(25, services, log_dump, void(int, const char*, const void*, size_t, int, void*))
DYNALIB_FN(26, services, log_enabled, int(int, const char*, void*))
DYNALIB_FN(27, services, log_level_name, const char*(int, void*))
DYNALIB_FN(28, services, log_set_callbacks, void(log_message_callback_type, log_write_callback_type, log_enabled_callback_type, void*))
DYNALIB_FN(29, services, set_thread_current_function_pointers, void(void*, void*, void*, void*, void*))
DYNALIB_FN(30, services, get_default_error_message, const char*(int, void*))
DYNALIB_FN(31, services, LED_SetCallbacks, void(LedCallbacks, void*))
DYNALIB_FN(32, services, led_set_status_active, void(LEDStatusData*, int, void*))
DYNALIB_FN(33, services, led_set_update_enabled, void(int, void*))
DYNALIB_FN(34, services, led_update_enabled, int(void*))
DYNALIB_FN(35, services, led_update, void(system_tick_t, LEDStatusData*, void*))

DYNALIB_FN(36, services, diag_register_source, int(const diag_source*, void*))
DYNALIB_FN(37, services, diag_enum_sources, int(diag_enum_sources_callback, size_t*, void*, void*))
DYNALIB_FN(38, services, diag_get_source, int(uint16_t, const diag_source**, void*))
DYNALIB_FN(39, services, diag_command, int(int, void*, void*))
// Export only on Photon and P1
#if PLATFORM_ID == 6 || PLATFORM_ID == 8
DYNALIB_FN(40, services, _printf_float, int(struct _reent*, struct _prt_data_t*, FILE*, int(*pfunc)(struct _reent* , FILE*, const char*, size_t), va_list*))

// Nanopb
// Helper functions
DYNALIB_FN(41, services, pb_ostream_init, pb_ostream_t*(void*))
DYNALIB_FN(42, services, pb_ostream_free, bool(pb_ostream_t*, void*))
DYNALIB_FN(43, services, pb_istream_init, pb_istream_t*(void*))
DYNALIB_FN(44, services, pb_istream_free, bool(pb_istream_t*, void*))
DYNALIB_FN(45, services, pb_ostream_from_buffer_ex, bool(pb_ostream_t*, pb_byte_t*, size_t, void*))
DYNALIB_FN(46, services, pb_istream_from_buffer_ex, bool(pb_istream_t*, const pb_byte_t*, size_t, void*))
// Encoding/decoding
DYNALIB_FN(47, services, pb_encode, bool(pb_ostream_t*, const pb_field_t[], const void*))
DYNALIB_FN(48, services, pb_get_encoded_size, bool(size_t*, const pb_field_t[], const void*))
DYNALIB_FN(49, services, pb_encode_tag_for_field, bool(pb_ostream_t*, const pb_field_t*))
DYNALIB_FN(50, services, pb_encode_submessage, bool(pb_ostream_t*, const pb_field_t[], const void*))
DYNALIB_FN(51, services, pb_decode_noinit, bool(pb_istream_t*, const pb_field_t[], void*))
DYNALIB_FN(52, services, pb_read, bool(pb_istream_t*, pb_byte_t*, size_t))
DYNALIB_FN(53, services, pb_encode_string, bool(pb_ostream_t*, const pb_byte_t*, size_t))
DYNALIB_FN(54, services, pb_encode_tag, bool(pb_ostream_t*, pb_wire_type_t, uint32_t))
DYNALIB_FN(55, services, pb_encode_varint, bool(pb_ostream_t*, pb_uint64_t))
# define BASE_IDX 56
#else
# define BASE_IDX 40
#endif

DYNALIB_FN(BASE_IDX + 0, services, set_error_message, void(const char*, ...))
DYNALIB_FN(BASE_IDX + 1, services, reset_error_message, void())
DYNALIB_FN(BASE_IDX + 2, services, get_error_message, const char*(int))

DYNALIB_END(services)

#undef BASE_IDX

#endif	/* SERVICES_DYNALIB_H */
