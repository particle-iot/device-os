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

// Default handler for CTRL_REQUEST_APP_CUSTOM requests
void __attribute((weak)) ctrl_request_custom_handler(ctrl_request* req) {
    system_ctrl_set_result(req, SYSTEM_ERROR_NOT_SUPPORTED, nullptr, nullptr, nullptr);
}

#if Wiring_LogConfig
// Callback invoked to process a logging configuration request
void(*log_process_ctrl_request_callback)(ctrl_request* req) = nullptr;
#endif

// Application handler for control requests
static void ctrl_request_handler(ctrl_request* req) {
    switch (req->type) {
#if Wiring_LogConfig
    case CTRL_REQUEST_LOG_CONFIG: {
        if (log_process_ctrl_request_callback) {
            log_process_ctrl_request_callback(req);
        } else {
            system_ctrl_set_result(req, SYSTEM_ERROR_NOT_SUPPORTED, nullptr, nullptr, nullptr);
        }
        break;
    }
#endif
    case CTRL_REQUEST_APP_CUSTOM: {
        ctrl_request_custom_handler(req);
        break;
    }
    default:
        system_ctrl_set_result(req, SYSTEM_ERROR_NOT_SUPPORTED, nullptr, nullptr, nullptr);
        break;
    }
}

void module_user_init_hook()
{
#if PLATFORM_BACKUP_RAM
    backup_ram_was_valid_ =  __backup_sram_signature==signature;
    if (!backup_ram_was_valid_) {
        system_initialize_user_backup_ram();
        __backup_sram_signature = signature;
    }
#endif

#if HAL_PLATFORM_RNG
    // Initialize the default stdlib PRNG using hardware RNG as a seed
    const uint32_t seed = HAL_RNG_GetRandomNumber();
    srand(seed);

    // If the user defines random_seed_from_cloud, call it with a seed value
    // generated by a hardware RNG as well.
    if (random_seed_from_cloud) {
        random_seed_from_cloud(seed);
    }
#endif
    // Register the random_seed_from_cloud handler
    spark_set_random_seed_from_cloud_handler(&random_seed_from_cloud, nullptr);

    // Register application handler for the control requests
    system_ctrl_set_app_request_handler(ctrl_request_handler, nullptr);
}
