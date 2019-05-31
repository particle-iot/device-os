/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "nfc_hal.h"

#if HAL_PLATFORM_NFC

#include "nfc_t2t_lib.h"
#include "app_error.h"
#include "service_debug.h"
#include "concurrent_hal.h"
#include "static_recursive_mutex.h"
#include <mutex>

static nfc_event_callback_t g_nfc_event_user_callback = NULL;
static void* g_context = NULL;
static bool g_nfc_enabled = false;

StaticRecursiveMutex s_nfcMutex;

class NfcLock {
    static void lock() {
        s_nfcMutex.lock();
    }

    static void unlock() {
        s_nfcMutex.unlock();
    }
};

static void nfc_type_2_callback(void* p_context, nfc_t2t_event_t event, const uint8_t* p_data, size_t data_length) {
    (void)p_context;
    switch (event) {
        case NFC_T2T_EVENT_FIELD_ON: {
            if (g_nfc_event_user_callback) {
                g_nfc_event_user_callback(NFC_EVENT_FIELD_ON, NULL, g_context);
            }
            break;
        }
        case NFC_T2T_EVENT_FIELD_OFF: {
            if (g_nfc_event_user_callback) {
                g_nfc_event_user_callback(NFC_EVENT_FIELD_OFF, NULL, g_context);
            }
            break;
        }
        case NFC_T2T_EVENT_DATA_READ: {
            if (g_nfc_event_user_callback) {
                g_nfc_event_user_callback(NFC_EVENT_READ, NULL, g_context);
            }
            break;
        }
        default:
            break;
    }
}

int hal_nfc_type2_init(void* reserved) {
    std::lock_guard<NfcLock> lk(NfcLock());

    uint32_t err_code = nfc_t2t_setup(nfc_type_2_callback, NULL);
    SPARK_ASSERT(err_code == NRF_SUCCESS)

    return 0;
}

int hal_nfc_type2_uninit(void* reserved) {
    std::lock_guard<NfcLock> lk(NfcLock());

    uint32_t err_code = nfc_t2t_done();
    SPARK_ASSERT(err_code == NRF_SUCCESS)

    return 0;
}

int hal_nfc_type2_set_payload(const void* msg_buf, size_t msg_len) {
    std::lock_guard<NfcLock> lk(NfcLock());

    uint32_t err_code = nfc_t2t_payload_set((uint8_t*)msg_buf, msg_len);
    SPARK_ASSERT(err_code == NRF_SUCCESS)

    return 0;
}

int hal_nfc_type2_start_emulation(void* reserved) {
    std::lock_guard<NfcLock> lk(NfcLock());

    if (g_nfc_enabled) {
        return 0;
    }

    uint32_t err_code = nfc_t2t_emulation_start();
    SPARK_ASSERT(err_code == NRF_SUCCESS)
    g_nfc_enabled = true;

    return 0;
}

int hal_nfc_type2_stop_emulation(void* reserved) {
    std::lock_guard<NfcLock> lk(NfcLock());

    if (g_nfc_enabled) {
        uint32_t err_code = nfc_t2t_emulation_stop();
        SPARK_ASSERT(err_code == NRF_SUCCESS)
        g_nfc_enabled = false;
    }

    return 0;
}

int hal_nfc_type2_set_callback(nfc_event_callback_t callback, void* context) {
    std::lock_guard<NfcLock> lk(NfcLock());

    g_nfc_event_user_callback = callback;
    g_context = context;

    return 0;
}

#endif
