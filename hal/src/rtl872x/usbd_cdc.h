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
#include "usbd_wcid.h"
#include "hal_platform.h"
#include "usb_hal.h"
#include "ringbuffer.h"
#include <functional>
#if !HAL_PLATFORM_USB_SOF
#include "concurrent_hal.h"
#endif // !HAL_PLATFORM_USB_SOF

namespace particle { namespace usbd {

namespace cdc {

#pragma pack(push, 1)
struct LineCoding {
    uint32_t dwDTERate;
    uint8_t bCharFormat;
    uint8_t bParityType;
    uint8_t bDataBits;
};
#pragma pack(pop)

enum Request {
    SET_LINE_CODING = 0x20,
    GET_LINE_CODING = 0x21,
    SET_CONTROL_LINE_STATE = 0x22,
    INVALID_REQUEST = 0xff
};

enum ControlSignal {
    DTR = 0x01,
    RTS = 0x02
};

const size_t MAX_CONTROL_PACKET_SIZE = 8;
const size_t MAX_DATA_PACKET_SIZE = 64;

} /* namespace cdc */

class CdcClassDriver : public particle::usbd::ClassDriver {
public:
    CdcClassDriver();
    ~CdcClassDriver();

    virtual int init(unsigned cfgIdx) override;
    virtual int deinit(unsigned cfgIdx) override;
    virtual int setup(particle::usbd::SetupRequest* req) override;
    virtual int startOfFrame() override;
    virtual int dataIn(unsigned ep, particle::usbd::EndpointEvent ev, size_t len) override;
    virtual int dataOut(unsigned ep, particle::usbd::EndpointEvent ev, size_t len) override;
    virtual int getConfigurationDescriptor(uint8_t* buf, size_t length, Speed speed = Speed::FULL, unsigned index = 0) override;
    virtual int getString(unsigned id, uint16_t langId, uint8_t* buf, size_t length) override;

    virtual int getNumInterfaces() const override;
    virtual int getNumStrings() const override;

    void setName(const char* name);

    typedef std::function<void (cdc::LineCoding lineCoding)> OnSetLineCodingCb;

    void onSetLineCoding(OnSetLineCodingCb cb);

    int initBuffers(void* rxBuffer, size_t rxBufferSize, void* txBuffer, size_t txBufferSize);
    int getLineCoding(cdc::LineCoding& lineCoding) const;
    int available();
    int availableForWrite();
    int read(uint8_t* buf, size_t len);
    int peek(uint8_t* buf, size_t len);
    int write(const uint8_t* buf, size_t len);
    void flush();
    bool isConnected() const;

    bool buffersConfigured() const;

    void useDummyIntEp(bool state);

protected:
    void setOpenState(bool state);

    int startRx();
    int startTx(bool holdoff = false);

#if !HAL_PLATFORM_USB_SOF
    int startTxTimeoutTimer();
    int stopTxTimeoutTimer();

    int startTxTimer();
    int stopTxTimer();
#endif // !HAL_PLATFORM_USB_SOF

private:
#if !HAL_PLATFORM_USB_SOF
    static void txTimeoutTimerCallback(os_timer_t timer);
    void txTimeoutTimerExpired();

    static void txTimerCallback(os_timer_t timer);
    void txTimerExpired();
#endif // !HAL_PLATFORM_USB_SOF

    int handleInSetupRequest(SetupRequest *req);
    int handleOutSetupRequest(SetupRequest *req);

private:
    char* name_ = nullptr;
    uint8_t epInData_ = ENDPOINT_INVALID;
    uint8_t epOutData_ = ENDPOINT_INVALID;
    uint8_t epInInt_ = ENDPOINT_INVALID;

    services::RingBuffer<uint8_t> txBuffer_;
    services::RingBuffer<uint8_t> rxBuffer_;

    volatile bool open_ = false;
    volatile bool rxState_ = false;
    volatile bool txState_ = false;

    uint8_t currentRequest_ = cdc::INVALID_REQUEST;
    size_t currentRequestLen_ = 0;

    uint8_t controlState_ = 0;

    __attribute__((aligned(4))) cdc::LineCoding lineCoding_ = {};
    __attribute__((aligned(4))) uint8_t cmdBuffer_[cdc::MAX_CONTROL_PACKET_SIZE] = {};

    OnSetLineCodingCb onSetLineCoding_;
    uint8_t altSetting_ = 0;

#if !HAL_PLATFORM_USB_SOF
    os_timer_t txTimeoutTimer_ = nullptr;
    os_timer_t txTimer_ = nullptr;
#endif // !HAL_PLATFORM_USB_SOF
    bool useDummyIntEp_ = false;
};

} } /* namespace particle::usbd */

