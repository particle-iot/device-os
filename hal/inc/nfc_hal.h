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

#pragma once

#include <stdint.h>

typedef enum {
    NFC_TAG_TYPE_NONE,
    NFC_TAG_TYPE_2,
    NFC_TAG_TYPE_NOT_SURPPORTED
} NfcTagType;

typedef enum {
    NFC_EVENT_FIELD_ON,             // NFC tag has detected external NFC field and was selected by an NFC polling device.
    NFC_EVENT_FIELD_OFF,            // External NFC field has been removed.
    NFC_EVENT_READ                  // NFC polling device has read all tag data.
} NfcEventType;

typedef struct {
    uint32_t reserved;
} NfcEvent;

typedef void (*NfcEventCallback)(NfcEventType type, NfcEvent *event);

int HAL_NFC_Type2_Init(NfcTagType type);
int HAL_NFC_Type2_Set_Payload(NfcTagType type, const uint8_t *msg_buf, uint16_t msg_len);
int HAL_NFC_Type2_Start_Emulation(NfcTagType type);
int HAL_NFC_Type2_Stop_Emulation(NfcTagType type);
int HAL_NFC_Type2_Set_Callback(NfcEventCallback callback);
