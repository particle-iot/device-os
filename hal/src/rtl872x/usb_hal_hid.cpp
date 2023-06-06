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

#include "usb_hal.h"
#include "usbd_hid.h"
#include "usbd_cdc.h"

using namespace particle::usbd;

extern CdcClassDriver& getCdcClassDriver();

void HAL_USB_HID_Init(uint8_t reserved, void* reserved1) {
    if (HidClassDriver::instance()->isEnabled()) {
        return;
    }
}

void HAL_USB_HID_Begin(uint8_t reserved, void* reserved1) {
    if (!HidClassDriver::instance()->isEnabled()) {
        HAL_USB_Detach();
        HidClassDriver::instance()->enable(true);
        getCdcClassDriver().useDummyIntEp(true);
        HAL_USB_Init();
        HAL_USB_Attach();
    }
}

void HAL_USB_HID_End(uint8_t reserved) {
    if (HidClassDriver::instance()->isEnabled()) {
        HAL_USB_Detach();
        HidClassDriver::instance()->enable(false);
        getCdcClassDriver().useDummyIntEp(false);
        HAL_USB_Attach();
    }
}

uint8_t HAL_USB_HID_Set_State(uint8_t id, uint8_t state, void* reserved) {
    if (HidClassDriver::instance()->isEnabled()) {
        HAL_USB_Detach();
        HidClassDriver::instance()->setDigitizerState(state);
        HAL_USB_Attach();
    }
    return 0;
}

void HAL_USB_HID_Send_Report(uint8_t reserved, void *pHIDReport, uint16_t reportSize, void* reserved1) {
    HidClassDriver::instance()->sendReport((const uint8_t*)pHIDReport, reportSize);
}

int32_t HAL_USB_HID_Status(uint8_t reserved, void* reserved1) {
    return HidClassDriver::instance()->transferStatus();
}
