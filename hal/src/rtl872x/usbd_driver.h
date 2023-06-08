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

#pragma once

#include "usbd_device.h"
#include "module_info.h"

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
#include "spark_wiring_thread.h"
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

// Avoid bringing in usbd_os.h
#define USB_HAL_H
extern "C" {
#include "usbd.h"
}

namespace particle { namespace usbd {

namespace rtl {

const size_t TEMP_BUFFER_SIZE = 512;

} // rtl

class RtlUsbDriver : public DeviceDriver {
public:
    static RtlUsbDriver* instance();

    virtual int attach() override;
    virtual int detach() override;
    virtual int suspend() override;
    virtual int resume() override;
    virtual DeviceState getState() const override;

    virtual int openEndpoint(unsigned ep, EndpointType type, size_t pakSize) override;
    virtual int closeEndpoint(unsigned ep) override;
    virtual int flushEndpoint(unsigned ep) override;
    virtual int stallEndpoint(unsigned ep) override;
    virtual int clearStallEndpoint(unsigned ep) override;
    virtual EndpointStatus getEndpointStatus(unsigned ep) override;
    virtual int setEndpointStatus(unsigned ep, EndpointStatus status) override;

    virtual int transferIn(unsigned ep, const uint8_t* ptr, size_t size) override;
    virtual int transferOut(unsigned ep, uint8_t* ptr, size_t size) override;

    virtual int setupReply(SetupRequest* r, const uint8_t* data, size_t size) override;
    virtual int setupReceive(SetupRequest* r, uint8_t* data, size_t size) override;
    virtual int setupError(SetupRequest* r) override;

    virtual unsigned getEndpointMask() const override;
    virtual unsigned updateEndpointMask(unsigned mask) const override;

    virtual bool lock() override;
    virtual void unlock() override;

    void halReadPacketFixup(void* ptr);

private:
    RtlUsbDriver();
    virtual ~RtlUsbDriver();

    static uint8_t* getDescriptorCb(usb_setup_req_t *req, usb_speed_type_t speed, uint16_t* len);
    static uint8_t setConfigCb(usb_dev_t* dev, uint8_t config);
    static uint8_t clearConfigCb(usb_dev_t* dev, uint8_t config);
    static uint8_t setupCb(usb_dev_t* dev, usb_setup_req_t* req);
    static uint8_t getClassDescriptorCb(usb_dev_t* dev, usb_setup_req_t* req);
    static uint8_t sofCb(usb_dev_t* dev);
    static uint8_t suspendCb(usb_dev_t* dev);
    static uint8_t resumeCb(usb_dev_t* dev);
    static uint8_t ep0DataInCb(usb_dev_t* dev);
    static uint8_t ep0DataOutCb(usb_dev_t* dev);
    static uint8_t epDataInCb(usb_dev_t* dev, uint8_t ep_num);
    static uint8_t epDataOutCb(usb_dev_t* dev, uint8_t ep_num, uint16_t len);

    void setDevReference(usb_dev_t* dev);
    void fixupReceivedData();

private:
    usb_dev_t* rtlDev_ = nullptr;
    bool setupError_ = false;

    const uint8_t RTL_USBD_ISR_PRIORITY = 7;
    __attribute__((aligned(4))) uint8_t tempBuffer_[rtl::TEMP_BUFFER_SIZE];

    // TODO: Validate whether these are required to be present at all times
    // or can be allocated on stack temporarily.
    usbd_config_t usbdCfg_ = {
        .max_ep_num = USBD_MAX_ENDPOINTS,
        .rx_fifo_size = USBD_MAX_RX_FIFO_SIZE,
        .nptx_fifo_size = USBD_MAX_NPTX_FIFO_SIZE,
        .ptx_fifo_size = USBD_MAX_PTX_FIFO_SIZE,
        .speed = USB_SPEED_FULL,
        .dma_enable = 0, // ?
        .self_powered = 1,
        .isr_priority = RTL_USBD_ISR_PRIORITY,
    };

    usbd_class_driver_t classDrv_ = {
        .get_descriptor = &getDescriptorCb,
        .set_config = &setConfigCb,
        .clear_config = &clearConfigCb,
        .setup = &setupCb,
        .get_class_descriptor = &getClassDescriptorCb,
        .clear_feature = nullptr,
        .sof = nullptr, // This is not delivered
        .suspend = &suspendCb,
        .resume = &resumeCb,
        .ep0_data_in = &ep0DataInCb,
        .ep0_data_out = &ep0DataOutCb,
        .ep_data_in = &epDataInCb,
        .ep_data_out = &epDataOutCb
    };

    void* fixupPtr_ = nullptr;
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    RecursiveMutex mutex_;
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

    volatile bool initialized_ = false;
};

} // namespace usbd
} // namespace particle
