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

#undef LOG_COMPILE_TIME_LEVEL
#include "logging.h"
#include "usbd_cdc.h"
#include <cstring>
#include <algorithm>
#include "usbd_wcid.h"
#include "appender.h"
#include "check.h"
#include "scope_guard.h"
#include "endian_util.h"
#include <algorithm>
#include <mutex>
#include "service_debug.h"

using namespace particle::usbd;

namespace {

const char DEFAULT_NAME[] = HAL_PLATFORM_USB_PRODUCT_STRING " " "USB Serial";
const uint8_t DUMMY_IN_EP = 0x8a;

} // anonymous

CdcClassDriver::CdcClassDriver()
        : ClassDriver() {
#if !HAL_PLATFORM_USB_SOF
    os_timer_create(&txTimeoutTimer_, HAL_PLATFORM_USB_CDC_TX_FAIL_TIMEOUT_MS, txTimeoutTimerCallback, this, true /* oneshot */, nullptr);
    SPARK_ASSERT(txTimeoutTimer_);
    os_timer_create(&txTimer_, HAL_PLATFORM_USB_CDC_TX_PERIOD_MS, txTimerCallback, this, true /* oneshot */, nullptr);
    SPARK_ASSERT(txTimer_);
#endif // !HAL_PLATFORM_USB_SOF
}

CdcClassDriver::~CdcClassDriver() {
    setName(nullptr);
#if !HAL_PLATFORM_USB_SOF
    if (txTimeoutTimer_) {
        os_timer_destroy(txTimeoutTimer_, nullptr);
        txTimeoutTimer_ = nullptr;
    }
    if (txTimer_) {
        os_timer_destroy(txTimer_, nullptr);
        txTimer_ = nullptr;
    }
#endif // !HAL_PLATFORM_USB_SOF
}

int CdcClassDriver::init(unsigned cfgIdx) {
    // deinit(cfgIdx);
    CHECK_TRUE(epInInt_ != ENDPOINT_INVALID, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(epOutData_ != ENDPOINT_INVALID, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(epInData_ != ENDPOINT_INVALID, SYSTEM_ERROR_INVALID_STATE);

    CHECK(dev_->openEndpoint(epInData_, EndpointType::BULK, cdc::MAX_DATA_PACKET_SIZE));
    CHECK(dev_->openEndpoint(epOutData_, EndpointType::BULK, cdc::MAX_DATA_PACKET_SIZE));
    if (!useDummyIntEp_) {
        CHECK(dev_->openEndpoint(epInInt_, EndpointType::INTERRUPT, cdc::MAX_CONTROL_PACKET_SIZE));
    }

    startRx();
    return 0;
}

int CdcClassDriver::deinit(unsigned cfgIdx) {
    if (isConfigured()) {
        if (txState_) {
            dev_->flushEndpoint(epInData_);
        }
    }
#if !HAL_PLATFORM_USB_SOF
    stopTxTimeoutTimer();
    stopTxTimer();
#endif // !HAL_PLATFORM_USB_SOF

    dev_->closeEndpoint(epInData_);
    dev_->closeEndpoint(epOutData_);
    dev_->closeEndpoint(epInInt_);

    epInInt_ = ENDPOINT_INVALID;
    epOutData_ = ENDPOINT_INVALID;
    epInData_ = ENDPOINT_INVALID;
    setOpenState(false);
    rxState_ = false;
    txState_ = false;
    rxBuffer_.reset();
    txBuffer_.reset();
    currentRequest_ = cdc::INVALID_REQUEST;
    currentRequestLen_ = 0;
    controlState_ = 0;
    memset(&lineCoding_, 0, sizeof(lineCoding_));
    altSetting_ = 0;
    return 0;
}

void CdcClassDriver::setOpenState(bool state) {
    if (state != open_) {
        if (state) {
#if !HAL_PLATFORM_USB_SOF
            stopTxTimeoutTimer();
#endif // !HAL_PLATFORM_USB_SOF
            txState_ = false;
            txBuffer_.reset();
            startRx();
            startTx();
        }
        open_ = state;
    }
}

void CdcClassDriver::setName(const char* name) {
    if (name_) {
        free(name_);
        name_ = nullptr;
    }
    if (name) {
        // Ignore allocation failures, we'll just default to DEFAULT_NAME
        name_ = strdup(name);
    }
}

int CdcClassDriver::setup(SetupRequest* req) {
    if (req->bmRequestTypeType != SetupRequest::TYPE_CLASS && req->bmRequestTypeType != SetupRequest::TYPE_STANDARD) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (req->wIndex != interfaceBase_ && req->wIndex != (interfaceBase_ + 1)) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    // Class or standard request
    if (req->bmRequestTypeType == SetupRequest::TYPE_CLASS) {
        if (req->bmRequestTypeDirection /* Device to host */) {
            return handleInSetupRequest(req);
        } else {
            currentRequest_ = req->bRequest;
            currentRequestLen_ = req->wLength;
            if (req->wLength) {
                return dev_->setupReceive(req, cmdBuffer_, std::min<size_t>(sizeof(cmdBuffer_), req->wLength));
            } else {
                return handleOutSetupRequest(req);
            }
        }
    } else {
        // Standard
        switch (req->bRequest) {
        case SetupRequest::REQUEST_GET_INTERFACE: {
            return dev_->setupReply(req, &altSetting_, sizeof(altSetting_));
        }
        case SetupRequest::REQUEST_SET_INTERFACE: {
            if (req->wValue == 0) {
                return dev_->setupReply(req, nullptr, 0);
            }
            break;
        }
        }
    }
    return SYSTEM_ERROR_INVALID_ARGUMENT;
}

int CdcClassDriver::handleInSetupRequest(SetupRequest* req) {
    switch (req->bRequest) {
    case cdc::GET_LINE_CODING: {
        return dev_->setupReply(req, (const uint8_t*)&lineCoding_, sizeof(lineCoding_));
    }
    }
    return SYSTEM_ERROR_INVALID_ARGUMENT;
}

int CdcClassDriver::handleOutSetupRequest(SetupRequest* req) {
    switch (req->bRequest) {
    case cdc::SET_LINE_CODING: {
        if (req->wLength >= sizeof(lineCoding_)) {
            memcpy(&lineCoding_, cmdBuffer_, sizeof(lineCoding_));
            if (onSetLineCoding_) {
                cdc::LineCoding lc = lineCoding_;
                lc.dwDTERate = littleEndianToNative(lc.dwDTERate);
                onSetLineCoding_(lc);
            }
        }
        return 0;
    }
    case cdc::SET_CONTROL_LINE_STATE: {
        if (req->wLength > 0) {
            controlState_ = cmdBuffer_[0];
        } else {
            controlState_ = req->wValue & 0xff;
        }
        if ((controlState_ & cdc::DTR) == cdc::DTR) {
            setOpenState(true);
        } else {
            setOpenState(false);
        }
        return 0;
    }
    }
    return SYSTEM_ERROR_INVALID_ARGUMENT;
}

int CdcClassDriver::dataIn(unsigned ep, particle::usbd::EndpointEvent ev, size_t len) {

    if (ep != epInData_ && ep != epInInt_) {
        return SYSTEM_ERROR_UNKNOWN;
    }

    if (!txState_) {
        return 0;
    }

#if !HAL_PLATFORM_USB_SOF
    stopTxTimeoutTimer();
#endif // !HAL_PLATFORM_USB_SOF

    if (txBuffer_.consumePending() != len) {
    }

    txBuffer_.consumeCommit(txBuffer_.consumePending());
    txState_ = false;


    startTx();

    return 0;
}

int CdcClassDriver::dataOut(unsigned ep, particle::usbd::EndpointEvent ev, size_t len) {
    if (ep == 0 && currentRequest_ != cdc::INVALID_REQUEST && currentRequestLen_ > 0) {
        SetupRequest req = {};
        req.bRequest = currentRequest_;
        req.wLength = currentRequestLen_;
        int r = handleOutSetupRequest(&req);
        currentRequest_ = cdc::INVALID_REQUEST;
        currentRequestLen_ = 0;
        return r;
    } else if (ep == epOutData_) {
        if (rxState_) {
            rxBuffer_.acquireCommit(len, rxBuffer_.acquirePending() - len);
            setOpenState(true);
            rxState_ = false;
            startRx();
            return 0;
        }
    }
    return SYSTEM_ERROR_UNKNOWN;
}

int CdcClassDriver::startOfFrame() {
    return 0;
}

int CdcClassDriver::getConfigurationDescriptor(uint8_t* buf, size_t length, Speed speed, unsigned index) {
    BufferAppender appender(buf, length);
    // Interface Association Descriptor
    appender.appendUInt8(0x08);   // bLength: Configuration Descriptor size
    appender.appendUInt8(0x0b);   // bDescriptorType: IAD
    appender.appendUInt8(interfaceBase_);   // bFirstInterface
    appender.appendUInt8(0x02);   // bInterfaceCount
    appender.appendUInt8(0x02);   // bFunctionClass
    appender.appendUInt8(0x02);   // bFunctionSubClass
    appender.appendUInt8(0x01);   // bFunctionProtocol
    appender.appendUInt8(stringBase_);   // iFunction

    // Interface Descriptor
    appender.appendUInt8(0x09);   // bLength: Interface Descriptor size
    appender.appendUInt8(DESCRIPTOR_INTERFACE);  // bDescriptorType: Interface
    appender.appendUInt8(interfaceBase_);   // bInterfaceNumber: Number of Interface
    appender.appendUInt8(0x00);   // bAlternateSetting: Alternate setting
    appender.appendUInt8(0x01);   // bNumEndpoints: One endpoints used
    appender.appendUInt8(0x02);   // bInterfaceClass: Communication Interface Class
    appender.appendUInt8(0x02);   // bInterfaceSubClass: Abstract Control Model
    appender.appendUInt8(0x01);   // bInterfaceProtocol: Common AT commands
    appender.appendUInt8(stringBase_);   // iInterface (using same string as iFunction in IAD)

    // Header Functional Descriptor
    appender.appendUInt8(0x05);   // bLength: Endpoint Descriptor size
    appender.appendUInt8(0x24);   // bDescriptorType: CS_INTERFACE
    appender.appendUInt8(0x00);   // bDescriptorSubtype: Header Func Desc
    appender.appendUInt8(0x10);   // bcdCDC: spec release number
    appender.appendUInt8(0x01);   // bcdCDC

    // Call Management Functional Descriptor
    appender.appendUInt8(0x05);   // bFunctionLength
    appender.appendUInt8(0x24);   // bDescriptorType: CS_INTERFACE
    appender.appendUInt8(0x01);   // bDescriptorSubtype: Call Management Func Desc
    appender.appendUInt8(0x00);   // bmCapabilities: D0+D1
    appender.appendUInt8(interfaceBase_ + 1);   // bDataInterface: 1

    // ACM Functional Descriptor
    appender.appendUInt8(0x04);   // bFunctionLength
    appender.appendUInt8(0x24);   // bDescriptorType: CS_INTERFACE
    appender.appendUInt8(0x02);   // bDescriptorSubtype: Abstract Control Management desc
    appender.appendUInt8(0x02);   // bmCapabilities

    // Union Functional Descriptor
    appender.appendUInt8(0x05);   // bFunctionLength
    appender.appendUInt8(0x24);   // bDescriptorType: CS_INTERFACE
    appender.appendUInt8(0x06);   // bDescriptorSubtype: Union func desc
    appender.appendUInt8(interfaceBase_ + 0);   // bMasterInterface: Communication class interface
    appender.appendUInt8(interfaceBase_ + 1);   // bSlaveInterface0: Data Class Interface

    uint8_t epInt = !useDummyIntEp_ ? CHECK(getNextAvailableEndpoint(ENDPOINT_IN)) : DUMMY_IN_EP;

    // Endpoint Descriptor (Interrupt)
    appender.appendUInt8(0x07);                  // bLength: Endpoint Descriptor size
    appender.appendUInt8(DESCRIPTOR_ENDPOINT);   // bDescriptorType: Endpoint
    appender.appendUInt8(epInt);                 // bEndpointAddress
    appender.appendUInt8(0x03);                  // bmAttributes: Interrupt
    appender.appendUInt16LE(cdc::MAX_CONTROL_PACKET_SIZE); // wMaxPacketSize
    // if (speed == Speed::HIGH) {
        appender.appendUInt8(0x10);              // bInterval
    // } else {
    //     appender.appendUInt8(0xFF);              // bInterval
    // }

    // Data class interface descriptor
    appender.appendUInt8(0x09);          // bLength: Endpoint Descriptor size
    appender.appendUInt8(DESCRIPTOR_INTERFACE);  // bDescriptorType:
    appender.appendUInt8(interfaceBase_ + 1);   // bInterfaceNumber: Number of Interface
    appender.appendUInt8(0x00);          // bAlternateSetting: Alternate setting
    appender.appendUInt8(0x02);          // bNumEndpoints: Two endpoints used
    appender.appendUInt8(0x0A);          // bInterfaceClass: CDC
    appender.appendUInt8(0x00);          // bInterfaceSubClass:
    appender.appendUInt8(0x00);          // bInterfaceProtocol:
    appender.appendUInt8(stringBase_);   // iInterface: Use the same string

    uint8_t epOutData = CHECK(getNextAvailableEndpoint(ENDPOINT_OUT));

    // Bulk OUT endpoint descriptor
    appender.appendUInt8(0x07);                  // bLength: Endpoint Descriptor size
    appender.appendUInt8(DESCRIPTOR_ENDPOINT);   // bDescriptorType: Endpoint
    appender.appendUInt8(epOutData);             // bEndpointAddress
    appender.appendUInt8(0x02);                  // bmAttributes: Bulk
    appender.appendUInt16LE(cdc::MAX_DATA_PACKET_SIZE); // wMaxPacketSize
    // if (speed == Speed::HIGH) {
        appender.appendUInt8(0x10);              // bInterval
    // } else {
    //     appender.appendUInt8(0xFF);              // bInterval
    // }

    uint8_t epInData = CHECK(getNextAvailableEndpoint(ENDPOINT_IN));
    // Bulk IN endpoint descriptor
    appender.appendUInt8(0x07);                  // bLength: Endpoint Descriptor size
    appender.appendUInt8(DESCRIPTOR_ENDPOINT);   // bDescriptorType: Endpoint
    appender.appendUInt8(epInData);              // bEndpointAddress
    appender.appendUInt8(0x02);                  // bmAttributes: Bulk
    appender.appendUInt16LE(cdc::MAX_DATA_PACKET_SIZE); // wMaxPacketSize
    // if (speed == Speed::HIGH) {
        appender.appendUInt8(0x10);              // bInterval
    // } else {
    //     appender.appendUInt8(0xFF);              // bInterval
    // }

    epInData_ = epInData;
    epOutData_ = epOutData;
    epInInt_ = epInt;


    return appender.dataSize();
}

int CdcClassDriver::getString(unsigned id, uint16_t langId, uint8_t* buf, size_t length) {
    if (id == stringBase_) {
        if (name_) {
            return dev_->getUnicodeString(name_, strlen(name_), buf, length);
        }
        return dev_->getUnicodeString(DEFAULT_NAME, sizeof(DEFAULT_NAME) - 1, buf, length);
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

int CdcClassDriver::startRx() {
    if (rxState_) {
        return 0;
    }

    // Updates current size
    rxBuffer_.acquireBegin();

    const size_t acquirable = rxBuffer_.acquirable();
    const size_t acquirableWrapped = rxBuffer_.acquirableWrapped();
    size_t rxSize = std::max(acquirable, acquirableWrapped);

    
    if (rxSize < cdc::MAX_DATA_PACKET_SIZE) {
        rxState_ = false;
        dev_->setEndpointStatus(epOutData_, EndpointStatus::NAK);
        return SYSTEM_ERROR_NO_MEMORY;
    }

    rxState_ = true;
    rxSize = cdc::MAX_DATA_PACKET_SIZE;
    auto ptr = rxBuffer_.acquire(rxSize);
    return dev_->transferOut(epOutData_, ptr, rxSize);
}

int CdcClassDriver::startTx(bool holdoff) {
    if (txState_) {
        return 0;
    }

    if (!open_) {
        return 0;
    }

    size_t consumable = txBuffer_.consumable();
    if (!consumable) {
        return 0;
    }

    if (holdoff) {
        stopTxTimer();
        return startTxTimer();
    }

    txState_ = true;
    consumable = std::min(consumable, cdc::MAX_DATA_PACKET_SIZE);
    auto buf = txBuffer_.consume(consumable);
    dev_->transferIn(epInData_, buf, consumable);

#if !HAL_PLATFORM_USB_SOF
    stopTxTimeoutTimer();
    startTxTimeoutTimer();
#endif // !HAL_PLATFORM_USB_SOF
    return 0;
}

#if !HAL_PLATFORM_USB_SOF
void CdcClassDriver::txTimeoutTimerCallback(os_timer_t timer) {
    void* timerId = nullptr;
    os_timer_get_id(timer, &timerId);
    if (timerId) {
        auto self = static_cast<CdcClassDriver*>(timerId);
        self->txTimeoutTimerExpired();
    }
}

void CdcClassDriver::txTimeoutTimerExpired() {
    std::lock_guard<Device> lk(*dev_);
    if (txState_ && open_) {
        setOpenState(false);
    }
    dev_->unlock();
    // FIXME: holding lock depending on the driver implementation may cause a deadlock
    dev_->flushEndpoint(epInData_);
    // Send ZLP (zero length packet)
    dev_->transferIn(epInData_, nullptr, 0);
    dev_->lock();
    txState_ = false;
    txBuffer_.reset();
    dev_->unlock();
}

void CdcClassDriver::txTimerCallback(os_timer_t timer) {
    void* timerId = nullptr;
    os_timer_get_id(timer, &timerId);
    if (timerId) {
        auto self = static_cast<CdcClassDriver*>(timerId);
        self->txTimerExpired();
    }
}

void CdcClassDriver::txTimerExpired() {
    std::lock_guard<Device> lk(*dev_);
    startTx();
}
#endif // !HAL_PLATFORM_USB_SOF

int CdcClassDriver::getNumInterfaces() const {
    return 2;
}

int CdcClassDriver::getNumStrings() const {
    return 1;
}

void CdcClassDriver::onSetLineCoding(OnSetLineCodingCb cb) {
    onSetLineCoding_ = cb;
}

int CdcClassDriver::initBuffers(void* rxBuffer, size_t rxBufferSize, void* txBuffer, size_t txBufferSize) {
    CHECK_FALSE(isEnabled(), SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(rxBuffer, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(rxBufferSize, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(txBuffer, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(txBufferSize, SYSTEM_ERROR_INVALID_ARGUMENT);
    if (dev_) {
        dev_->lock();
    }
    rxBuffer_.init((uint8_t*)rxBuffer, rxBufferSize);
    txBuffer_.init((uint8_t*)txBuffer, txBufferSize);
    if (dev_) {
        dev_->unlock();
    }
    return 0;
}

int CdcClassDriver::getLineCoding(cdc::LineCoding& lineCoding) const {
    CHECK_TRUE(isConfigured(), SYSTEM_ERROR_INVALID_STATE);
    std::lock_guard<Device> lk(*dev_);
    lineCoding = lineCoding_;
    lineCoding.dwDTERate = littleEndianToNative(lineCoding.dwDTERate);
    return 0;
}

int CdcClassDriver::available() {
    CHECK_TRUE(isConfigured(), SYSTEM_ERROR_INVALID_STATE);
    std::lock_guard<Device> lk(*dev_);
    return rxBuffer_.data();
}

int CdcClassDriver::availableForWrite() {
    CHECK_TRUE(isConfigured(), SYSTEM_ERROR_INVALID_STATE);
    std::lock_guard<Device> lk(*dev_);
    return txBuffer_.space();
}

int CdcClassDriver::read(uint8_t* buf, size_t len) {
    CHECK_TRUE(isConfigured(), SYSTEM_ERROR_INVALID_STATE);
    const size_t maxRead = CHECK(available());
    const size_t readSize = std::min(maxRead, len);
    CHECK_TRUE(readSize > 0, SYSTEM_ERROR_NO_MEMORY);
    std::lock_guard<Device> lk(*dev_);
    size_t r = CHECK(rxBuffer_.get(buf, readSize));
    startRx();
    return r;
}

int CdcClassDriver::peek(uint8_t* buffer, size_t size) {
    CHECK_TRUE(isConfigured(), SYSTEM_ERROR_INVALID_STATE);
    const size_t maxRead = CHECK(available());
    const size_t peekSize = std::min(maxRead, size);
    CHECK_TRUE(peekSize > 0, SYSTEM_ERROR_NO_MEMORY);
    std::lock_guard<Device> lk(*dev_);
    return rxBuffer_.peek(buffer, peekSize);
}

int CdcClassDriver::write(const uint8_t* buf, size_t len) {
    CHECK_TRUE(isConfigured(), SYSTEM_ERROR_INVALID_STATE);
    const size_t canWrite = CHECK(availableForWrite());
    const size_t writeSize = std::min(canWrite, len);
    CHECK_TRUE(writeSize > 0, SYSTEM_ERROR_NO_MEMORY);
    {
        std::lock_guard<Device> lk(*dev_);
        CHECK(txBuffer_.put(buf, writeSize));
        startTx(true /* holdoff */);
    }
    return writeSize;
}

void CdcClassDriver::flush() {
    while (isConfigured() && open_ && txState_) {
        {
            std::lock_guard<Device> lk(*dev_);
            if (txBuffer_.empty()) {
                return;
            }
        }
    }
}

bool CdcClassDriver::isConnected() const {
    return open_;
}

bool CdcClassDriver::buffersConfigured() const {
    return rxBuffer_.size() && txBuffer_.size();
}

#if !HAL_PLATFORM_USB_SOF
int CdcClassDriver::startTxTimeoutTimer() {
    return os_timer_change(txTimeoutTimer_, OS_TIMER_CHANGE_START, false, HAL_PLATFORM_USB_CDC_TX_FAIL_TIMEOUT_MS, 0, nullptr);
}

int CdcClassDriver::stopTxTimeoutTimer() {
    return os_timer_change(txTimeoutTimer_, OS_TIMER_CHANGE_STOP, false, 0, 0, nullptr);
}

int CdcClassDriver::startTxTimer() {
    return os_timer_change(txTimer_, OS_TIMER_CHANGE_START, false, HAL_PLATFORM_USB_CDC_TX_PERIOD_MS, 0, nullptr);
}

int CdcClassDriver::stopTxTimer() {
    return os_timer_change(txTimer_, OS_TIMER_CHANGE_STOP, false, 0, 0, nullptr);
}
#endif // !HAL_PLATFORM_USB_SOF

void CdcClassDriver::useDummyIntEp(bool state) {
    useDummyIntEp_ = state;
}
