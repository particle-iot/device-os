/*
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

#include "spark_wiring_platform.h"
#include "spark_wiring_ble.h"

#if Wiring_BLE

int AdvertisingDataManagement::locateAdStructure(uint8_t adsType, const uint8_t* data, uint16_t len, uint16_t* offset, uint16_t* adsLen) {
    // A valid AD structure is composed of Length field, Type field and Data field.
    // Each field should be filled with at least one byte.
    for (uint16_t i = 0; (i + 3) <= len; i = i) {
        *adsLen = data[i];

        uint8_t type = data[i + 1];
        if (type == adsType) {
            // The value of adsLen doesn't include the length field of an AD structure.
            if ((i + *adsLen + 1) <= len) {
                *offset = i;
                *adsLen += 1;
                return SYSTEM_ERROR_NONE;
            }
            else {
                return SYSTEM_ERROR_INTERNAL;
            }
        }
        else {
            // Navigate to the next AD structure.
            i += (*adsLen + 1);
        }
    }

    return SYSTEM_ERROR_NOT_FOUND;
}

int AdvertisingDataManagement::encodeAdStructure(uint8_t adsType, const uint8_t* data, uint16_t len, bool sr, uint8_t* advData, uint16_t* advDataLen) {
    uint16_t  offset;
    uint16_t  adsLen;
    bool      adsExist = false;

    if (sr && adsType == BLE_SIG_AD_TYPE_FLAGS) {
        // The scan response data should not contain the AD Flags AD structure.
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    if (locateAdStructure(adsType, advData, *advDataLen, &offset, &adsLen) == SYSTEM_ERROR_NONE) {
        adsExist = true;
    }

    if (data == NULL) {
        // The advertising data must contain the AD Flags AD structure.
        if (adsType == BLE_SIG_AD_TYPE_FLAGS) {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }

        // Remove the AD structure from advertising data.
        if (adsExist == true) {
            uint16_t moveLen = *advDataLen - offset - adsLen;
            memcpy(&advData[offset], &advData[offset + adsLen], moveLen);
            *advDataLen = *advDataLen - adsLen;
        }

        return SYSTEM_ERROR_NONE;
    }
    else if (adsExist) {
        // Update the existing AD structure.
        uint16_t staLen = *advDataLen - adsLen;
        if ((staLen + len + 2) <= BLE_MAX_ADV_DATA_LEN) {
            // Firstly, move the last consistent block.
            uint16_t moveLen = *advDataLen - offset - adsLen;
            memmove(&advData[offset + len], &advData[offset + adsLen], moveLen);

            // Secondly, Update the AD structure.
            // The Length field is the total length of Type field and Data field.
            advData[offset + 1] = len + 1;
            memcpy(&advData[offset + 2], data, len);

            // An AD structure is composed of one byte length field, one byte Type field and Data field.
            *advDataLen = staLen + len + 2;
            return SYSTEM_ERROR_NONE;
        }
        else {
            return SYSTEM_ERROR_LIMIT_EXCEEDED;
        }
    }
    else {
        // Append the AD structure at the and of advertising data.
        if ((*advDataLen + len + 2) <= BLE_MAX_ADV_DATA_LEN) {
            advData[(*advDataLen)++] = len + 1;
            advData[(*advDataLen)++] = adsType;
            memcpy(&advData[*advDataLen], data, len);
            *advDataLen += len;
            return SYSTEM_ERROR_NONE;
        }
        else {
            return SYSTEM_ERROR_LIMIT_EXCEEDED;
        }
    }
}


int BLEGAPClass::decodeAdvertisingData(uint8_t adsType, const uint8_t* advData, uint16_t advDataLen, uint8_t* data, uint16_t* len) {
    // An AD structure must consist of 1 byte length field, 1 byte type field and at least 1 byte data field
    if (advData == NULL || advDataLen < 3) {
        *len = 0;
        return SYSTEM_ERROR_NOT_FOUND;
    }

    uint16_t dataOffset, dataLen;
    if (locateAdStructure(adsType, advData, advDataLen, &dataOffset, &dataLen) == SYSTEM_ERROR_NONE) {
        if (len != NULL) {
            dataLen = dataLen - 2;
            if (data != NULL && *len > 0) {
                // Only copy the data field of the found AD structure.
                *len = MIN(*len, dataLen);
                memcpy(data, &advData[dataOffset + 2], *len);
            }
            *len = dataLen;
        }

        return SYSTEM_ERROR_NONE;
    }

    *len = 0;
    return SYSTEM_ERROR_NOT_FOUND;
}

#endif
