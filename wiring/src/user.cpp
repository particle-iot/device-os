/**
 ******************************************************************************
 * @file    user.cpp
 * @authors Matthew McGowan
 * @date    13 February 2015
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

#include "system_user.h"
#include "system_control.h"
#include <stddef.h>
#include <string.h>
#include "spark_wiring_platform.h"
#include "spark_wiring_usbserial.h"
#include "spark_wiring_usartserial.h"
#include "spark_wiring_watchdog.h"
#include "spark_wiring_logging.h"
#include "rng_hal.h"



/**
 * Declare the following function bodies as weak. They will only be used if no
 * other strong function body is found when linking.
 */
void setup() __attribute((weak));
void loop() __attribute((weak));

// needed on ARM GCC
#if PLATFORM_ID!=3
/**
 * Declare weak setup/loop implementations so that they are always defined.
 */

void setup()  {

}


void loop() {

}
#endif

/**
 * Allow the application to override this to avoid processing
 * overhead when serial events are not required.
 */
void serialEventRun() __attribute__((weak));

void serialEvent() __attribute__((weak));
void serialEvent1() __attribute__((weak));
void usbSerialEvent1() __attribute__((weak));

#if PLATFORM_ID==3
// gcc doesn't allow weak functions to not exist, so they must be defined.
__attribute__((weak)) void serialEvent() {}
__attribute__((weak)) void serialEvent1() {}
#endif

#if Wiring_Serial2
void serialEvent2() __attribute__((weak));
#endif

#if Wiring_Serial3
void serialEvent3() __attribute__((weak));
#endif

#if Wiring_Serial4
void serialEvent4() __attribute__((weak));
#endif

#if Wiring_Serial5
void serialEvent5() __attribute__((weak));
#endif

void _post_loop()
{
	serialEventRun();
	application_checkin();
}

/**
 * Provides background processing of serial data.
 */
void serialEventRun()
{
    if (serialEvent && Serial.available()>0)
        serialEvent();

    if (serialEvent1 && Serial1.available()>0)
        serialEvent1();

#if Wiring_Serial2
    if (serialEventRun2) serialEventRun2();
#endif

#if Wiring_Serial3
    if (serialEventRun3) serialEventRun3();
#endif

#if Wiring_Serial4
    if (serialEventRun4) serialEventRun4();
#endif

#if Wiring_Serial5
    if (serialEventRun5) serialEventRun5();
#endif

#if Wiring_USBSerial1
    if (usbSerialEvent1 && USBSerial1.available()>0)
        usbSerialEvent1();
#endif
}

#if defined(STM32F2XX)
#define PLATFORM_BACKUP_RAM 1
#else
#define PLATFORM_BACKUP_RAM 0
#endif

#if PLATFORM_BACKUP_RAM
extern char link_global_retained_initial_values;
extern char link_global_retained_start;
extern char link_global_retained_end;

/**
 * Initializes the user region of the backup ram.
 * This is provided here so it can be called from the monolithic firmware or from
 * the dynamically linked application module.
 */
void system_initialize_user_backup_ram()
{
    size_t len = &link_global_retained_end-&link_global_retained_start;
    memcpy(&link_global_retained_start, &link_global_retained_initial_values, len);
}

#include "platform_headers.h"

static retained volatile uint32_t __backup_sram_signature;
static bool backup_ram_was_valid_ = false;
const uint32_t signature = 0x9A271C1E;

bool __backup_ram_was_valid() { return backup_ram_was_valid_; }

#else

bool __backup_ram_was_valid() { return false; }

#endif

#if Wiring_LogConfig
// Callback invoked to process USB request for logging configuration
bool(*log_process_config_request_callback)(char*, size_t, size_t, size_t*, DataFormat) = nullptr;
#endif

#ifdef USB_VENDOR_REQUEST_ENABLE

// Synchronous handler for customizable requests (USBRequestType::USB_REQUEST_CUSTOM)
bool __attribute((weak)) usb_request_custom_handler(char* buf, size_t buf_size, size_t req_size, size_t* rep_size) {
    return false;
}

bool usb_request_app_handler(USBRequest* req, void* reserved) {
    switch (req->type) {
#if Wiring_LogConfig
    case USB_REQUEST_LOG_CONFIG: {
        if (!log_process_config_request_callback || !log_process_config_request_callback(req->data, USB_REQUEST_BUFFER_SIZE,
                req->request_size, &req->reply_size, (DataFormat)req->format)) {
            return false;
        }
        system_set_usb_request_result(req, USB_REQUEST_RESULT_OK, nullptr);
        return true;
    }
#endif
    case USB_REQUEST_CUSTOM: {
        if (!usb_request_custom_handler(req->data, USB_REQUEST_BUFFER_SIZE, req->request_size, &req->reply_size)) {
            return false;
        }
        system_set_usb_request_result(req, USB_REQUEST_RESULT_OK, nullptr);
        return true;
    }
    default:
        return false; // Unsupported request type
    }
}

#endif // USB_VENDOR_REQUEST_ENABLE

void module_user_init_hook()
{
#if PLATFORM_BACKUP_RAM
    backup_ram_was_valid_ =  __backup_sram_signature==signature;
    if (!backup_ram_was_valid_) {
        system_initialize_user_backup_ram();
        __backup_sram_signature = signature;
    }
#endif

    /* for dynamically linked user part, set the random seed if the user
     * app defines random_seed_from_cloud.
     */
// todo - add a RNG define for that capability
#if defined(STM32F2XX)
    if (random_seed_from_cloud) {
    		uint32_t seed = HAL_RNG_GetRandomNumber();
    		random_seed_from_cloud(seed);
    }
#endif

#ifdef USB_VENDOR_REQUEST_ENABLE
    system_set_usb_request_app_handler(usb_request_app_handler, nullptr);
#endif
}
