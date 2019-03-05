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
#include "usb_hal.h"

#ifdef DYNALIB_EXPORT
#include "system_mode.h"
#include "system_sleep.h"
#include "system_task.h"
#include "system_update.h"
#include "system_event.h"
#include "system_version.h"
#include "system_control.h"
#include "system_led_signal.h"
#include "system_setup.h"
#endif

DYNALIB_BEGIN(system)

DYNALIB_FN(0, system, system_mode, System_Mode_TypeDef(void))
DYNALIB_FN(1, system, set_system_mode, void(System_Mode_TypeDef))

DYNALIB_FN(2, system, set_ymodem_serial_flash_update_handler, void(ymodem_serial_flash_update_handler))
DYNALIB_FN(3, system, system_firmwareUpdate, bool(Stream*, void*))
DYNALIB_FN(4, system, system_fileTransfer, bool(system_file_transfer_t*, void*))

DYNALIB_FN(5, system, system_delay_ms, void(unsigned long, bool))
DYNALIB_FN(6, system, system_sleep, int(Spark_Sleep_TypeDef, long, uint32_t, void*))
DYNALIB_FN(7, system, system_sleep_pin, int(uint16_t, uint16_t, long, uint32_t, void*))
DYNALIB_FN(8, system, system_subscribe_event, int(system_event_t, system_event_handler_t*, void*))
DYNALIB_FN(9, system, system_unsubscribe_event, void(system_event_t, system_event_handler_t*, void*))
DYNALIB_FN(10, system, system_button_pushed_duration, uint16_t(uint8_t, void*))
DYNALIB_FN(11, system, system_thread_set_state, void(spark::feature::State, void*))
DYNALIB_FN(12, system, system_version_info, int(SystemVersionInfo*, void*))
DYNALIB_FN(13, system, system_internal, void*(int item, void*))
DYNALIB_FN(14, system, system_set_flag, int(system_flag_t, uint8_t, void*))
DYNALIB_FN(15, system, system_get_flag, int(system_flag_t, uint8_t*, void*))
DYNALIB_FN(16, system, Spark_Prepare_For_Firmware_Update, int(FileTransfer::Descriptor&, uint32_t, void*))
DYNALIB_FN(17, system, Spark_Save_Firmware_Chunk, int(FileTransfer::Descriptor&, const uint8_t*, void*))
DYNALIB_FN(18, system, Spark_Finish_Firmware_Update, int(FileTransfer::Descriptor&, uint32_t, void*))

DYNALIB_FN(19, system, application_thread_current, uint8_t(void*))
DYNALIB_FN(20, system, system_thread_current, uint8_t(void*))
DYNALIB_FN(21, system, application_thread_invoke, uint8_t(void(*)(void*), void*, void*))
DYNALIB_FN(22, system, system_thread_get_state, spark::feature::State(void*))
DYNALIB_FN(23, system, system_notify_time_changed, void(uint32_t, void*, void*))
DYNALIB_FN(24, system, main_thread_current, uint8_t(void*))

#if defined(USB_VENDOR_REQUEST_ENABLE) && HAL_PLATFORM_KEEP_DEPRECATED_APP_USB_REQUEST_HANDLERS
DYNALIB_FN(25, system, system_set_usb_request_app_handler, void(void*, void*)) // Deprecated
DYNALIB_FN(26, system, system_set_usb_request_result, void(void*, int, void*)) // Deprecated
#define BASE_IDX 27
#else
#define BASE_IDX 25
#endif // defined(USB_VENDOR_REQUEST_ENABLE) && HAL_PLATFORM_KEEP_DEPRECATED_APP_USB_REQUEST_HANDLERS

DYNALIB_FN(BASE_IDX + 0, system, led_start_signal, int(int, uint8_t, int, void*))
DYNALIB_FN(BASE_IDX + 1, system, led_stop_signal, void(int, int, void*))
DYNALIB_FN(BASE_IDX + 2, system, led_signal_started, int(int, void*))
DYNALIB_FN(BASE_IDX + 3, system, led_set_signal_theme, int(const LEDSignalThemeData*, int, void*))
DYNALIB_FN(BASE_IDX + 4, system, led_get_signal_theme, int(LEDSignalThemeData*, int, void*))
DYNALIB_FN(BASE_IDX + 5, system, led_signal_status, const LEDStatusData*(int, void*))
DYNALIB_FN(BASE_IDX + 6, system, led_pattern_period, uint16_t(int, int, void*))
DYNALIB_FN(BASE_IDX + 7, system, system_set_tester_handlers, int(system_tester_handlers_t*, void*))
DYNALIB_FN(BASE_IDX + 8, system, system_format_diag_data, int(const uint16_t*, size_t, unsigned, appender_fn, void*, void*))

// Control requests
DYNALIB_FN(BASE_IDX + 9, system, system_ctrl_set_app_request_handler, int(ctrl_request_handler_fn, void*))
DYNALIB_FN(BASE_IDX + 10, system, system_ctrl_alloc_reply_data, int(ctrl_request*, size_t, void*))
DYNALIB_FN(BASE_IDX + 11, system, system_ctrl_free_request_data, void(ctrl_request*, void*))
DYNALIB_FN(BASE_IDX + 12, system, system_ctrl_set_result, void(ctrl_request*, int, ctrl_completion_handler_fn, void*, void*))

DYNALIB_FN(BASE_IDX + 13, system, system_pool_alloc, void*(size_t, void*))
DYNALIB_FN(BASE_IDX + 14, system, system_pool_free, void(void*, void*))
DYNALIB_FN(BASE_IDX + 15, system, system_sleep_pins, int(const uint16_t*, size_t, const InterruptMode*, size_t, long, uint32_t, void*))
DYNALIB_FN(BASE_IDX + 16, system, system_invoke_event_handler, int(uint16_t handlerInfoSize, FilteringEventHandler* handlerInfo, const char* event_name, const char* event_data, void* reserved))


DYNALIB_END(system)

#undef BASE_IDX

#endif	/* SYSTEM_DYNALIB_H */
