/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#include "usbd_device.h"
#include "usbd_driver.h"
#include "usbd_dfu.h"
#include "usbd_dfu_mal.h"

#include "logging.h"
#include "hw_config.h"
#include "dfu_hal.h"
#include "usbd_wcid.h"

#include "hal_irq_flag.h"
#include <algorithm>

using namespace particle::usbd;

namespace {

#define LOBYTE(x)  ((uint8_t)(x & 0x00FF))
#define HIBYTE(x)  ((uint8_t)((x & 0xFF00) >>8))

const uint8_t sDeviceDescriptor[] = {
  0x12,                       /*bLength */
  DESCRIPTOR_DEVICE,          /*bDescriptorType*/
  0x00,                       /*bcdUSB */
  0x02,
  0x00,                       /*bDeviceClass*/
  0x00,                       /*bDeviceSubClass*/
  0x00,                       /*bDeviceProtocol*/
  USB_MAX_EP0_SIZE,           /*bMaxPacketSize*/
  LOBYTE(USBD_VID_PARTICLE),     /*idVendor*/
  HIBYTE(USBD_VID_PARTICLE),     /*idVendor*/
  LOBYTE(USBD_PID_DFU),       /*idProduct*/
  HIBYTE(USBD_PID_DFU),       /*idProduct*/
  LOBYTE(0x0252),             /*bcdDevice (2.52) */
  HIBYTE(0x0252),             /*bcdDevice (2.52) */
  STRING_IDX_MANUFACTURER,    /*Index of manufacturer  string*/
  STRING_IDX_PRODUCT,         /*Index of product string*/
  STRING_IDX_SERIAL,          /*Index of serial number string*/
  0x01                        /*bNumConfigurations*/
};

} /* anonymous */

extern "C" {
// To simplify access from assembly to avoid mangling
extern void* rtwUsbBackupSp;
extern void* rtwUsbBackupLr;
extern void rtw_usb_task();
} // extern "C"

Device gUsbDevice;
dfu::DfuClassDriver gDfuInstance(&gUsbDevice);

/* FIXME: move somewhere else */
dfu::mal::InternalFlashMal gInternalFlashMal;
dfu::mal::DctMal gDctMal;

void HAL_DFU_USB_Init(void) {
  gUsbDevice.setDeviceDescriptor(sDeviceDescriptor, sizeof(sDeviceDescriptor));
  gUsbDevice.registerDriver(RtlUsbDriver::instance());
  gUsbDevice.registerClass(&gDfuInstance);
  gDfuInstance.enable(true);
  /* Register DFU MALs */
  gDfuInstance.registerMal(0, &gInternalFlashMal);
  gDfuInstance.registerMal(1, &gDctMal);
  gUsbDevice.attach();
}

extern "C" void __attribute__((naked)) HAL_DFU_Process() {
  asm volatile(
    // Save stack pointer and link register
    // They might be used by rtw_down_sema() to break out of endless USB task loop
    "mov r0, sp\n\t"
    "mov r1, lr\n\t"
    "ldr r3, =rtwUsbBackupSp\n\t"
    "str r0, [r3]\n\t"
    "ldr r3, =rtwUsbBackupLr\n\t"
    "str r1, [r3]\n\t"
    // Call into rtw_usb_task in order not to write sFunc/sCtx checks manually in assembly
    "bl rtw_usb_task\n\t"
    // Return
    "bx lr\n\t");
}

void DFU_Check_Reset(void) {
  if (gDfuInstance.checkReset()) {
    gUsbDevice.detach();
    Finish_Update();
  }
}
