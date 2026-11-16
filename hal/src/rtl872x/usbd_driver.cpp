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

#include "usbd_driver.h"
#include "system_error.h"
#include "service_debug.h"
#include <mutex>
#include <algorithm>
#include <cstring>
#include "scope_guard.h"
#include "timer_hal.h"
#include "rtl8721d_usb.h"

using namespace particle::usbd;

#define CHECK_RTL_USB(_expr) \
        ({ \
            const auto _ret = _expr; \
            if (_ret < 0) { \
                return HAL_ERR_HW; \
            } \
            _ret; \
        })

#define CHECK_RTL_USB_TO_SYSTEM(_expr) \
        ({ \
            const auto _ret = _expr; \
            if (_ret != HAL_OK) { \
                return SYSTEM_ERROR_UNKNOWN; \
            } \
            _ret; \
        })


namespace {

SetupRequest sLastUsbSetupRequest = {};

Speed rtlSpeedToDriver(usb_speed_type_t speed) {
    switch (speed) {
    case USB_SPEED_HIGH: {
        return Speed::HIGH;
    }
    case USB_SPEED_LOW: {
        return Speed::LOW;
    }
    case USB_SPEED_FULL:
    default: {
        return Speed::FULL;
    }
    }
}

uint8_t endpointTypeToRtl(EndpointType type) {
    switch (type) {
    case EndpointType::CONTROL: {
        return USB_CH_EP_TYPE_CTRL;
    }
    case EndpointType::INTERRUPT: {
        return USB_CH_EP_TYPE_INTR;
    }
    case EndpointType::ISOCHRONOUS: {
        return USB_CH_EP_TYPE_ISOC;
    }
    case EndpointType::BULK: {
        return USB_CH_EP_TYPE_BULK;
    }
    }
    return 0;
}

// The SDK's lock in the PCD (no header for it)
const size_t RTL_USB_PCD_SPINLOCK_OFFSET = 0x180;

const unsigned int RTL_USB_RESET_COUNT_THRESHOLD = 100;

const system_tick_t RTL_USB_EP0_WATCHDOG_TIMEOUT = 3000;
const unsigned RTL_USB_EP0_STUCK_REPLY_SETUPS = 3;
const system_tick_t RTL_USB_TX_FIFO_EMPTY_STORM_TIMEOUT = 10;

// Buffer the SDK reads SETUPs into (no header; from the disassembly of the SDK)
const size_t RTL_USB_PCD_SETUP_BUF_OFFSET = 0x188;
// SDK per-endpoint transfer records in the PCD (no header for it; from the disassembly of usbd_pcd_isr_task): IN
// endpoint n is RTL_USB_PCD_EP_SIZE bytes at RTL_USB_PCD_IN_EP_OFFSET + n * size
const size_t RTL_USB_PCD_IN_EP_OFFSET = 0x18;
const size_t RTL_USB_PCD_EP_SIZE = 0x24;
const size_t RTL_USB_PCD_EP_XFER_LEN_OFFSET = 0x14;
const size_t RTL_USB_PCD_EP_XFER_COUNT_OFFSET = 0x18;
// Polls of GINTSTS.GINAKEFF / DIEPINT.EPDISD when setting the global IN NAK / disabling an IN endpoint
const unsigned RTL_USB_EP_DISABLE_POLLS = 10000;
// Non-periodic transmit request queue depth (GHWCFG2.NPTXQDEPTH)
const uint32_t RTL_USB_NPTX_QUEUE_DEPTH = 8;

// Scoped hold of the SDK's PCD lock (usb_os_spinlock() is a FreeRTOS mutex, not a critical section)
class SdkLock {
public:
    explicit SdkLock(void* pcd)
            : lock_(*(usb_spinlock_t**)((uint8_t*)pcd + RTL_USB_PCD_SPINLOCK_OFFSET)) {
        usb_os_spinlock(lock_);
    }

    ~SdkLock() {
        usb_os_spinunlock(lock_);
    }

    SdkLock(const SdkLock&) = delete;
    SdkLock& operator=(const SdkLock&) = delete;

private:
    usb_spinlock_t* lock_;
};

bool waitForBits(volatile u32* reg, u32 bits) {
    for (unsigned i = 0; i < RTL_USB_EP_DISABLE_POLLS; i++) {
        if (*reg & bits) {
            return true;
        }
    }
    return false;
}

bool nonPeriodicTxFifoEmpty() {
    const uint32_t empty = (RTL_USB_NPTX_QUEUE_DEPTH << USB_OTG_GNPTXSTS_NPTQXSAV_Pos) | (USB_GLOBAL->GNPTXFSIZ >> 16);
    return (USB_GLOBAL->GNPTXSTS & (USB_OTG_GNPTXSTS_NPTQXSAV | USB_OTG_GNPTXSTS_NPTXFSAV)) == empty;
}

bool isNonPeriodicInEndpoint(unsigned ep) {
    const uint32_t type = USB_INEP(ep)->DIEPCTL & USB_OTG_DIEPCTL_EPTYP;
    return type == 0 || type == USB_OTG_DIEPCTL_EPTYP_1;
}

} // anonymous

extern "C" {
uint8_t usbd_pcd_ep_set_stall(void* pcd, uint8_t ep);
uint8_t usbd_pcd_ep_clear_stall(void* pcd, uint8_t ep);
uint8_t usbd_pcd_ep_flush(void* pcd, uint8_t ep);
uint8_t usb_hal_flush_tx_fifo(uint32_t num);
uint8_t usb_hal_flush_rx_fifo();
uint8_t usb_hal_write_packet(uint8_t *src, uint8_t ep_ch_num, uint16_t len);
void usb_hal_disable_global_interrupt(void);
void usb_hal_enable_global_interrupt(void);
void usbd_pcd_set_address(void* pcd, uint8_t addr);
void usbd_pcd_stop(void* pcd);
void usbd_pcd_start(void* pcd);

u32 __real_usb_hal_read_interrupts(void);
void __real_usb_hal_clear_interrupts(u32 interrupt);

u32 __wrap_usb_hal_read_interrupts(void) {
    uint32_t val =  __real_usb_hal_read_interrupts();
    if (val & USB_OTG_GINTSTS_ENUMDNE) {
        // XXX: Looks like RX/TX FIFOs are not correctly re-initialized on bus reset
        // or when we perform flushEndpoint()
        // Reset the stack manually
        RtlUsbDriver::instance()->reset();
    }
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    if ((val & USB_OTG_GINTSTS_NPTXFE) && RtlUsbDriver::instance()->txFifoEmptyStormCheck()) {
        // Masked: nothing for the SDK's handler to do on this pass
        val &= ~USB_OTG_GINTSTS_NPTXFE;
    }
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    return val;
}

void __wrap_usb_hal_clear_interrupts(u32 interrupt) {
    __real_usb_hal_clear_interrupts(interrupt);
}

u8 __real_usbd_core_setup_stage(usb_dev_t* dev, u8* setup);

u8 __wrap_usbd_core_setup_stage(usb_dev_t* dev, u8* setup) {
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    RtlUsbDriver::instance()->setupStage(dev);
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    return __real_usbd_core_setup_stage(dev, setup);
}

// FIXME: This is a nasty workaround for the SDK USB driver
// where it fails to deliver vendor requests with recipient=interface

int usb_hal_read_packet(void* ptr, uint32_t size, void* unknown);
int __real_usb_hal_read_packet(void* ptr, uint32_t size, void* unknown);

int __wrap_usb_hal_read_packet(void* ptr, uint32_t size, void* unknown) {
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    if (size == sizeof(sLastUsbSetupRequest)) {
        RtlUsbDriver::instance()->cancelControlTransferOnSetup(ptr);
    }
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    int r = __real_usb_hal_read_packet(ptr, size, unknown);
    bool fixed = false;
    if (size == sizeof(sLastUsbSetupRequest)) {
        auto req = static_cast<SetupRequest*>(ptr);
        memcpy(&sLastUsbSetupRequest, req, sizeof(sLastUsbSetupRequest));
        if (req->bmRequestTypeRecipient == SetupRequest::RECIPIENT_INTERFACE) {
            // Patch recipient to be device instead of interface
            req->bmRequestTypeType = SetupRequest::TYPE_CLASS;
            fixed = true;
            RtlUsbDriver::instance()->halReadPacketFixup(ptr);
        }
    }
    if (!fixed) {
        RtlUsbDriver::instance()->halReadPacketFixup(nullptr);
    }
    return r;
}

} // extern "C"

RtlUsbDriver* RtlUsbDriver::instance() {
    static RtlUsbDriver driver;
    return &driver;
}

RtlUsbDriver::RtlUsbDriver() {
}

RtlUsbDriver::~RtlUsbDriver() {
}

int RtlUsbDriver::attach() {
    if (initialized_) {
        // Ignore
        return 0;
    }
    initialized_ = true;
    // NOTE: these calls may fail
    auto r = usbd_init(&usbdCfg_);
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    SCOPE_GUARD({
        if (!thread_) {
            os_thread_create(&thread_, "usbd_watch", rtl::RTL_USBD_ISR_PROCESSING_THREAD_PRIORITY, &RtlUsbDriver::loop, this, OS_THREAD_STACK_SIZE_DEFAULT);
            SPARK_ASSERT(thread_);
        }
    });
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    if (r) {
        initialized_ = false;
        CHECK_RTL_USB_TO_SYSTEM(r);
    } else {
        usbd_register_class(&classDrv_);
    }

    return 0;
}

int RtlUsbDriver::detach() {
    std::lock_guard<RtlUsbDriver> lk(*this);
    needsReset_ = false;
    initialized_ = false;
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    HAL_Delay_Milliseconds(10);
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

    usb_hal_disable_global_interrupt();
    usbd_unregister_class();
    usbd_deinit();
    resetCount_ = 0;
    cancelledInEps_ = 0;
    ep0CancelStreak_ = 0;
    txFifoEmptyIdle_ = false;
    txFifoEmptyMasked_ = false;
    return 0;
}

bool RtlUsbDriver::reattach() {
    std::lock_guard<RtlUsbDriver> lk(*this);
    detach();
    attach();
    return initialized_;
}

void RtlUsbDriver::reset() {
    if (config_ != 0 || resetCount_++ >= RTL_USB_RESET_COUNT_THRESHOLD) {
        requestReattach();
    }
}

void RtlUsbDriver::requestReattach() {
    config_ = 0;
    usb_hal_disable_global_interrupt();
    needsReset_ = true;
    resetCount_ = 0;
}

void RtlUsbDriver::loop(void* ctx) {
    auto self = static_cast<RtlUsbDriver*>(ctx);
    constexpr auto RTL_USB_WATCH_PERIOD = 1000;
    static bool error = false;

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    while (true) {
        if (!self->initialized_ && !error) {
            HAL_Delay_Milliseconds(RTL_USB_WATCH_PERIOD / 10);
            continue;
        }
        HAL_Delay_Milliseconds(RTL_USB_WATCH_PERIOD);
        if (self->needsReset_ || error) {
            error = !self->reattach();
        }
        self->watch();
    }
#else
    // Polled from DFU_Check_Reset() in the bootloader's main loop, not run in a dedicated thread.
    static system_tick_t last = HAL_Timer_Get_Milli_Seconds();
    const system_tick_t now = HAL_Timer_Get_Milli_Seconds();
    if ((self->initialized_ || error) && now - last >= RTL_USB_WATCH_PERIOD) {
        last = now;
        if (self->needsReset_ || error) {
            error = !self->reattach();
        }
    }
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
}

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
void RtlUsbDriver::watch() {
    bool nakCleared = false;
    bool ep0Wedged = false;
    {
        std::lock_guard<RtlUsbDriver> lk(*this);
        if (!initialized_ || !rtlDev_) {
            return;
        }
        SdkLock sdk(pcd());
        nakCleared = clearGlobalNak();
        if (txFifoEmptyMasked_) {
            txFifoEmptyMasked_ = false;
            USB_GLOBAL->GINTMSK |= USB_OTG_GINTMSK_NPTXFEM;
        }
        ep0Wedged = ep0WatchdogCheck();
    }
    if (nakCleared) {
        LOG(WARN, "Global NAK stuck, cleared");
    }
    if (ep0Wedged) {
        LOG(WARN, "EP0 not progressing for %u ms, reattaching", (unsigned)RTL_USB_EP0_WATCHDOG_TIMEOUT);
    }
}

bool RtlUsbDriver::clearGlobalNak() {
    const uint32_t dctl = USB_DEVICE->DCTL;
    if (!(dctl & (USB_OTG_DCTL_GINSTS | USB_OTG_DCTL_GONSTS))) {
        return false;
    }
    if (dctl & USB_OTG_DCTL_GINSTS) {
        cancelledInEps_ |= stopInTransfers(1UL << 0) & ~(1UL << 0);
    }
    USB_DEVICE->DCTL |= USB_OTG_DCTL_CGINAK | USB_OTG_DCTL_CGONAK;
    return true;
}

bool RtlUsbDriver::ep0WatchdogCheck() {
    const system_tick_t now = HAL_Timer_Get_Milli_Seconds();
    const bool nak = (USB_INEP(0)->DIEPINT & USB_OTG_DIEPINT_NAK) || (USB_OUTEP(0)->DOEPINT & USB_OTG_DOEPINT_NAK);
    USB_INEP(0)->DIEPINT = USB_OTG_DIEPINT_NAK;
    USB_OUTEP(0)->DOEPINT = USB_OTG_DOEPINT_NAK;
    const unsigned setups = ep0Setups_;
    const bool progress = setups != ep0LastSetups_;
    ep0LastSetups_ = setups;
    const bool replyStuck = ep0CancelStreak_ >= RTL_USB_EP0_STUCK_REPLY_SETUPS &&
            now - ep0CancelStreakStart_ >= RTL_USB_EP0_WATCHDOG_TIMEOUT;
    if (config_ == 0 || needsReset_ || (!replyStuck && (progress || !nak))) {
        ep0WaitStart_ = now;
        return false;
    }
    if (!replyStuck && now - ep0WaitStart_ < RTL_USB_EP0_WATCHDOG_TIMEOUT) {
        return false;
    }
    requestReattach();
    return true;
}

void RtlUsbDriver::setupStage(usb_dev_t* dev) {
    ep0Setups_++;
    const uint32_t eps = __atomic_exchange_n(&cancelledInEps_, 0, __ATOMIC_SEQ_CST);
    for (unsigned ep = 1; eps && ep < USBD_MAX_ENDPOINTS; ep++) {
        if (eps & (1UL << ep)) {
            epDataInCb(dev, ep);
        }
    }
}

bool RtlUsbDriver::isSetupBuffer(const void* ptr) const {
    return rtlDev_ && ptr == *((void**)((uint8_t*)pcd() + RTL_USB_PCD_SETUP_BUF_OFFSET));
}

void RtlUsbDriver::cancelControlTransferOnSetup(void* ptr) {
    if (!isSetupBuffer(ptr)) {
        return;
    }
    if (!(USB_INEP(0)->DIEPCTL & USB_OTG_DIEPCTL_EPENA)) {
        ep0CancelStreak_ = 0;
        return;
    }
    if (ep0CancelStreak_++ == 0) {
        ep0CancelStreakStart_ = HAL_Timer_Get_Milli_Seconds();
    }
    cancelledInEps_ |= stopInTransfers(1UL << 0) & ~(1UL << 0);
}

bool RtlUsbDriver::txFifoEmptyStormCheck() {
    if (!nonPeriodicTxFifoEmpty() || (USB_INEP(0)->DIEPCTL & USB_OTG_DIEPCTL_EPENA)) {
        txFifoEmptyIdle_ = false;
        return false;
    }
    const system_tick_t now = HAL_Timer_Get_Milli_Seconds();
    if (!txFifoEmptyIdle_) {
        txFifoEmptyIdle_ = true;
        txFifoEmptyStart_ = now;
        return false;
    }
    if (now - txFifoEmptyStart_ < RTL_USB_TX_FIFO_EMPTY_STORM_TIMEOUT) {
        return false;
    }
    txFifoEmptyIdle_ = false;
    txFifoEmptyMasked_ = true;
    USB_GLOBAL->GINTMSK &= ~USB_OTG_GINTMSK_NPTXFEM;
    return true;
}
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

uint32_t RtlUsbDriver::stopInTransfers(uint32_t eps) {
    const bool flush = !nonPeriodicTxFifoEmpty();
    if (flush) {
        for (unsigned ep = 0; ep < USBD_MAX_ENDPOINTS; ep++) {
            if (isNonPeriodicInEndpoint(ep)) {
                eps |= 1UL << ep;
            }
        }
    }
    USB_DEVICE->DCTL |= USB_OTG_DCTL_SGINAK;
    if (!waitForBits(&USB_GLOBAL->GINTSTS, USB_OTG_GINTSTS_GINAKEFF)) {
        USB_DEVICE->DCTL |= USB_OTG_DCTL_CGINAK;
        return 0;
    }
    uint32_t stopped = 0;
    for (unsigned ep = 0; ep < USBD_MAX_ENDPOINTS; ep++) {
        if (!(eps & (1UL << ep))) {
            continue;
        }
        auto in = USB_INEP(ep);
        const bool enabled = in->DIEPCTL & USB_OTG_DIEPCTL_EPENA;
        if (enabled) {
            in->DIEPCTL |= USB_OTG_DIEPCTL_SNAK | USB_OTG_DIEPCTL_EPDIS;
            waitForBits(&in->DIEPINT, USB_OTG_DIEPINT_EPDISD);
            in->DIEPINT = USB_OTG_DIEPINT_EPDISD;
        }
        auto sdkEp = (uint8_t*)pcd() + RTL_USB_PCD_IN_EP_OFFSET + ep * RTL_USB_PCD_EP_SIZE;
        auto len = (volatile uint32_t*)(sdkEp + RTL_USB_PCD_EP_XFER_LEN_OFFSET);
        auto count = (volatile uint32_t*)(sdkEp + RTL_USB_PCD_EP_XFER_COUNT_OFFSET);
        if (*count < *len) {
            *count = *len;
            if (enabled) {
                stopped |= 1UL << ep;
            }
        }
    }
    if (flush) {
        usb_hal_flush_tx_fifo(0);
    }
    USB_DEVICE->DCTL |= USB_OTG_DCTL_CGINAK;
    return stopped;
}

int RtlUsbDriver::suspend() {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int RtlUsbDriver::resume() {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

DeviceState RtlUsbDriver::getState() const {
    if (rtlDev_) {
        switch (rtlDev_->dev_state) {
        case USBD_STATE_INIT: {
            return DeviceState::DETACHED;
        }
        case USBD_STATE_DEFAULT: {
            return DeviceState::DEFAULT;
        }
        case USBD_STATE_ADDRESSED: {
            return DeviceState::ADDRESSED;
        }
        case USBD_STATE_CONFIGURED: {
            return DeviceState::CONFIGURED;
        }
        case USBD_STATE_SUSPENDED: {
            return DeviceState::SUSPENDED;
        }
        }
    }

    return DeviceState::NONE;
}

int RtlUsbDriver::openEndpoint(unsigned ep, EndpointType type, size_t pakSize) {
    SPARK_ASSERT(rtlDev_);
    return CHECK_RTL_USB_TO_SYSTEM(usbd_ep_init(rtlDev_, ep, endpointTypeToRtl(type), pakSize));
}

int RtlUsbDriver::closeEndpoint(unsigned ep) {
    SPARK_ASSERT(rtlDev_);
    if ((ep & 0x7f) >= USBD_MAX_ENDPOINTS) {
        return SYSTEM_ERROR_OUT_OF_RANGE;
    }
    return CHECK_RTL_USB_TO_SYSTEM(usbd_ep_deinit(rtlDev_, ep));
}

int RtlUsbDriver::flushEndpoint(unsigned ep) {
    SPARK_ASSERT(rtlDev_);
    std::lock_guard<RtlUsbDriver> lk(*this);
    if (ep & SetupRequest::DIRECTION_DEVICE_TO_HOST) {
        SdkLock sdk(pcd());
        stopInTransfers(1UL << (ep & 0x7f));
    }
    return 0;
}

int RtlUsbDriver::stallEndpoint(unsigned ep) {
    SPARK_ASSERT(rtlDev_);
    return CHECK_RTL_USB_TO_SYSTEM(usbd_pcd_ep_set_stall(pcd(), ep));
}

int RtlUsbDriver::clearStallEndpoint(unsigned ep) {
    SPARK_ASSERT(rtlDev_);
    return CHECK_RTL_USB_TO_SYSTEM(usbd_pcd_ep_clear_stall(pcd(), ep));
}

EndpointStatus RtlUsbDriver::getEndpointStatus(unsigned ep) {
    // FIXME: there is an undocumented field in usbd_ep_t
    return EndpointStatus::NONE;
}

int RtlUsbDriver::setEndpointStatus(unsigned ep, EndpointStatus status) {
    // FIXME: there is an undocumented field in usbd_ep_t
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int RtlUsbDriver::endpointTransferComplete(unsigned ep) {
    if (ep & SetupRequest::DIRECTION_DEVICE_TO_HOST) {
        return (USB_INEP(ep & 0x7f)->DIEPINT & USB_OTG_DIEPINT_XFRC) ? 1 : 0;
    }
    return (USB_OUTEP(ep & 0x7f)->DOEPINT & USB_OTG_DOEPINT_XFRC) ? 1 : 0;
}

int RtlUsbDriver::endpointTransferRemaining(unsigned ep) {
    if (ep & SetupRequest::DIRECTION_DEVICE_TO_HOST) {
        return USB_INEP(ep & 0x7f)->DIEPTSIZ & USB_OTG_DIEPTSIZ_XFRSIZ;
    }
    return USB_OUTEP(ep & 0x7f)->DOEPTSIZ & USB_OTG_DOEPTSIZ_XFRSIZ;
}

int RtlUsbDriver::transferIn(unsigned ep, const uint8_t* ptr, size_t size) {
    SPARK_ASSERT(rtlDev_);
    std::lock_guard<RtlUsbDriver> lk(*this);
    if (ptr == nullptr && size == 0) {
        // FIXME
        SdkLock sdk(pcd());
        return CHECK_RTL_USB_TO_SYSTEM(usb_hal_write_packet(nullptr, ep & 0x7f, 0));
    }
    return CHECK_RTL_USB_TO_SYSTEM(usbd_ep_transmit(rtlDev_, ep | SetupRequest::DIRECTION_DEVICE_TO_HOST, (uint8_t*)ptr, size));
}

int RtlUsbDriver::transferOut(unsigned ep, uint8_t* ptr, size_t size) {
    SPARK_ASSERT(rtlDev_);
    auto self = instance();
    std::lock_guard<RtlUsbDriver> lk(*self);
    return CHECK_RTL_USB_TO_SYSTEM(usbd_ep_receive(rtlDev_, ep & ~(SetupRequest::DIRECTION_DEVICE_TO_HOST), ptr, size));
}

int RtlUsbDriver::setupReply(SetupRequest* r, const uint8_t* data, size_t size) {
    SPARK_ASSERT(rtlDev_);
    if (data && size) {
        if (r) {
            // We cannot send more than r->wLength, otherwise the host will
            // get OVERFLOW error
            size = std::min<size_t>(size, r->wLength);
        }
        if (size <= rtl::TEMP_BUFFER_SIZE) {
            // For data that fits into temp buffer, we'll use it to simplify
            // some common cases of returning not a lot of data generated on stack
            memcpy(tempBuffer_, data, size);
            CHECK_RTL_USB_TO_SYSTEM(usbd_ep0_transmit(rtlDev_, tempBuffer_, size));
        } else {
            CHECK_RTL_USB_TO_SYSTEM(usbd_ep0_transmit(rtlDev_, (uint8_t*)data, size));
        }
    } else if (data == nullptr && size == 0) {
        // FIXME: doesn't work
        // CHECK_RTL_USB_TO_SYSTEM(usbd_ep0_transmit_status(rtlDev_));
    }
    return 0;
}


int RtlUsbDriver::setupReceive(SetupRequest* r, uint8_t* data, size_t size) {
    SPARK_ASSERT(rtlDev_);
    if (data && size) {
        CHECK_RTL_USB_TO_SYSTEM(usbd_ep0_receive(rtlDev_, data, size));
    } else if (data == nullptr && size == 0) {
        // FIXME: doesn't work
        // CHECK_RTL_USB_TO_SYSTEM(usbd_ep0_receive_status(rtlDev_));
    }
    return 0;
}

int RtlUsbDriver::setupError(SetupRequest* r) {
    setupError_ = true;
    return 0;
}

unsigned RtlUsbDriver::getEndpointMask() const {
    // EP0-INOUT, EP1-IN, EP2-OUT, EP3-IN, EP4-OUT
    unsigned maskIn = (1 << (ENDPOINT_MASK_OUT_BITPOS + 0) /* EP1 */) | (1 << (ENDPOINT_MASK_OUT_BITPOS + 2) /* EP 3 */);
    unsigned maskOut = (1 << 1 /* EP2 */) | (1 << 3 /* EP 4*/);
    return maskIn | maskOut;
}

unsigned RtlUsbDriver::updateEndpointMask(unsigned mask) const {
    return mask;
}

uint8_t* RtlUsbDriver::getDescriptorCb(usb_setup_req_t *req, usb_speed_type_t speed, uint16_t* len) {
    auto self = instance();
    std::lock_guard<RtlUsbDriver> lk(*self);

    uint8_t type = req->wValue >> 8;
    auto s = rtlSpeedToDriver(speed);
    int r = SYSTEM_ERROR_UNKNOWN;
    if (type != DESCRIPTOR_STRING) {
        r = self->getDescriptor((DescriptorType)type, self->tempBuffer_, rtl::TEMP_BUFFER_SIZE, s, req->wValue & 0xff);
    } else {
        r = self->getString(req->wValue & 0xff, req->wIndex, self->tempBuffer_, rtl::TEMP_BUFFER_SIZE);
    }
    if (r < 0) {
        *len = 0;
        return nullptr;
    }
    *len = (uint16_t)r;
    return self->tempBuffer_;
}

uint8_t RtlUsbDriver::getClassDescriptorCb(usb_dev_t* dev, usb_setup_req_t* req) {
    return setupCb(dev, req);
}

uint8_t RtlUsbDriver::setConfigCb(usb_dev_t* dev, uint8_t config) {
    auto self = instance();

    std::lock_guard<RtlUsbDriver> lk(*self);
    self->setDevReference(dev);
    self->config_ = (int)config;
    self->resetCount_ = 0;
    return CHECK_RTL_USB(self->setConfig(config));
}

uint8_t RtlUsbDriver::clearConfigCb(usb_dev_t* dev, uint8_t config) {
    auto self = instance();

    std::lock_guard<RtlUsbDriver> lk(*self);
    self->setDevReference(dev);
    return CHECK_RTL_USB(self->clearConfig(config));
}

uint8_t RtlUsbDriver::setupCb(usb_dev_t* dev, usb_setup_req_t* req) {
    auto self = instance();
    std::lock_guard<RtlUsbDriver> lk(*self);

    self->setDevReference(dev);
    // No need to fixup for setup requests
    self->halReadPacketFixup(nullptr);
    static_assert(sizeof(SetupRequest) == sizeof(usb_setup_req_t), "SetupRequest and usb_setup_req_t are expected to be of the same size");
    CHECK_RTL_USB(self->setupRequest(&sLastUsbSetupRequest));
    if (self->setupError_) {
        self->setupError_ = false;
        return HAL_ERR_HW;
    }
    // ?
    // if (req->wLength == 0) {
    //     if (req->bmRequestType & SetupRequest::DIRECTION_DEVICE_TO_HOST) {
    //         usbd_ep0_transmit_status(dev);
    //     } else {
    //         usbd_ep0_receive_status(dev);
    //     }
    // }
    return 0;
}

uint8_t RtlUsbDriver::sofCb(usb_dev_t* dev) {
    auto self = instance();
    std::lock_guard<RtlUsbDriver> lk(*self);

    self->setDevReference(dev);
    return CHECK_RTL_USB(self->startOfFrame());
}

uint8_t RtlUsbDriver::suspendCb(usb_dev_t* dev) {
    auto self = instance();
    std::lock_guard<RtlUsbDriver> lk(*self);

    self->setDevReference(dev);
    return CHECK_RTL_USB(self->notifySuspend());
}

uint8_t RtlUsbDriver::resumeCb(usb_dev_t* dev) {
    auto self = instance();
    std::lock_guard<RtlUsbDriver> lk(*self);

    self->setDevReference(dev);
    return CHECK_RTL_USB(self->notifyResume());
}

uint8_t RtlUsbDriver::ep0DataInCb(usb_dev_t* dev) {
    auto self = instance();
    std::lock_guard<RtlUsbDriver> lk(*self);

    self->setDevReference(dev);
    CHECK_RTL_USB(self->notifyEpTransferDone(SetupRequest::DIRECTION_DEVICE_TO_HOST | 0x00, EndpointEvent::DONE, dev->ep0_data_len));
    return 0;
}

uint8_t RtlUsbDriver::ep0DataOutCb(usb_dev_t* dev) {
    auto self = instance();
    std::lock_guard<RtlUsbDriver> lk(*self);

    self->setDevReference(dev);
    self->fixupReceivedData();
    CHECK_RTL_USB(self->notifyEpTransferDone(SetupRequest::DIRECTION_HOST_TO_DEVICE | 0x00, EndpointEvent::DONE, dev->ep0_data_len));
    return 0;
}

uint8_t RtlUsbDriver::epDataInCb(usb_dev_t* dev, uint8_t ep_num) {
    auto self = instance();
    std::lock_guard<RtlUsbDriver> lk(*self);

    self->setDevReference(dev);
    // NOTE: no way to tell the size of the last transfer
    CHECK_RTL_USB(self->notifyEpTransferDone(SetupRequest::DIRECTION_DEVICE_TO_HOST | ep_num, EndpointEvent::DONE, 0 /* dev->ep_in[ep_num & 0x7f].data_len */));
    return 0;
}

uint8_t RtlUsbDriver::epDataOutCb(usb_dev_t* dev, uint8_t ep_num, uint16_t len) {
    auto self = instance();
    std::lock_guard<RtlUsbDriver> lk(*self);

    self->setDevReference(dev);
    self->fixupReceivedData();
    CHECK_RTL_USB(self->notifyEpTransferDone(SetupRequest::DIRECTION_HOST_TO_DEVICE | ep_num, EndpointEvent::DONE, len));
    return 0;
}

void RtlUsbDriver::setDevReference(usb_dev_t* dev) {
    if (!rtlDev_) {
        rtlDev_ = dev;
    }
}

void RtlUsbDriver::halReadPacketFixup(void* ptr) {
    fixupPtr_ = ptr;
}

void RtlUsbDriver::fixupReceivedData() {
    if (fixupPtr_) {
        auto req = static_cast<SetupRequest*>(fixupPtr_);
        memcpy(req, &sLastUsbSetupRequest, sizeof(sLastUsbSetupRequest));
        fixupPtr_ = nullptr;
    }
}

bool RtlUsbDriver::lock() {
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    mutex_.lock();
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    return true;
}

void RtlUsbDriver::unlock() {
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    mutex_.unlock();
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
}
