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
#include "nfc_t2t_lib.h"
#include "app_error.h"

static nfc_event_callback_t g_nfc_event_user_callback = NULL;

static void nfc_type_2_callback(void * p_context, nfc_t2t_event_t event, const uint8_t * p_data, size_t data_length) {
    (void)p_context;
    switch (event) {
        case NFC_T2T_EVENT_FIELD_ON: {
            if (g_nfc_event_user_callback) {
                g_nfc_event_user_callback(NFC_EVENT_FIELD_ON, NULL, NULL);
            }
            break;
        }
        case NFC_T2T_EVENT_FIELD_OFF: {
            if (g_nfc_event_user_callback) {
                g_nfc_event_user_callback(NFC_EVENT_FIELD_OFF, NULL, NULL);
            }
            break;
        }
        case NFC_T2T_EVENT_DATA_READ: {
            if (g_nfc_event_user_callback) {
                g_nfc_event_user_callback(NFC_EVENT_READ, NULL, NULL);
            }
            break;
        }
        default:
            break;
    }
}

int hal_nfc_type2_init(void) {
    uint32_t err_code = nfc_t2t_setup(nfc_type_2_callback, NULL);
    APP_ERROR_CHECK(err_code);

    return 0;
}

int hal_nfc_type2_set_payload(const void *msg_buf, size_t msg_len) {
    uint32_t err_code = nfc_t2t_payload_set((uint8_t *)msg_buf, msg_len);
    APP_ERROR_CHECK(err_code);

    return 0;
}

int hal_nfc_type2_start_emulation(void) {
    uint32_t err_code = nfc_t2t_emulation_start();
    APP_ERROR_CHECK(err_code);

    return 0;
}

int hal_nfc_type2_stop_emulation(void) {
    uint32_t err_code = nfc_t2t_emulation_stop();
    APP_ERROR_CHECK(err_code);

    return 0;
}

int hal_nfc_type2_set_callback(nfc_event_callback_t callback) {
    g_nfc_event_user_callback = callback;

    return 0;
}
