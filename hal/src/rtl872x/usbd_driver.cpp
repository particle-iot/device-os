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

// EP0 watchdog: how long the host may keep waiting on EP0 without a control transfer
// completing before EP0 is repaired, and again after a repair before the device reattaches.
const system_tick_t RTL_USB_EP0_WATCHDOG_TIMEOUT = 3000;
// SETUP packets EP0 OUT can take back to back (the host may retry a SETUP before firmware re-arms)
const uint32_t RTL_USB_EP0_SETUP_COUNT = 3;

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
// Back-to-back TX FIFO empty interrupts with nothing written that count as a storm. The handler loops at about 100k
// passes per second, so this is about 10 ms: longer than a host normally takes to poll a pending IN endpoint.
const unsigned RTL_USB_TX_FIFO_EMPTY_STORM_PASSES = 1000;
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

u32 __real_usbd_hal_read_out_ep_interrupts(void* pcd, u8 ep);
u32 __real_usbd_hal_read_in_ep_interrupts(void* pcd, u8 ep);

u32 __wrap_usbd_hal_read_out_ep_interrupts(void* pcd, u8 ep) {
    const u32 val = __real_usbd_hal_read_out_ep_interrupts(pcd, ep);
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    if (ep == 0) {
        RtlUsbDriver::instance()->ep0TransferEvent(val & USB_OTG_DOEPINT_STUP, false, val & USB_OTG_DOEPINT_XFRC);
    }
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    return val;
}

u32 __wrap_usbd_hal_read_in_ep_interrupts(void* pcd, u8 ep) {
    const u32 val = __real_usbd_hal_read_in_ep_interrupts(pcd, ep);
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    if (ep == 0) {
        RtlUsbDriver::instance()->ep0TransferEvent(false, val & USB_OTG_DIEPINT_XFRC, false);
    }
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    return val;
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
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
        RtlUsbDriver::instance()->ep0SetupRead(ptr);
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
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
    txFifoEmptyMasked_ = false;
    ep0Repaired_ = false;
    return 0;
}

bool RtlUsbDriver::reattach() {
    std::lock_guard<RtlUsbDriver> lk(*this);
    detach();
    attach();
    reattaches_++;
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
        std::lock_guard<RtlUsbDriver> lk(*self);
        if (self->initialized_) {
            self->completeCancelledInTransfers();
            self->txFifoEmptyRearm();
            self->ep0WatchdogCheck();
        }
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
// Called by the USB task with the SDK lock held. A control transfer counts as completed when its status stage
// completes: an IN transfer complete if the status stage is IN, an OUT one if it is OUT. ep0SetupRead() records the
// direction when the SDK reads the SETUP. A SETUP packet completes an OUT transfer itself, between its read and its
// SETUP phase done, and that one doesn't count. Transfer completes without a SETUP before them (e.g. an EP0 OUT
// completion storm) don't count either.
void RtlUsbDriver::ep0SetupRead(const void* ptr) {
    if (!isSetupBuffer(ptr)) {
        return;
    }
    ep0SetupRead_ = true;
    // The status stage is IN unless the host reads data, then it is OUT
    auto req = static_cast<const SetupRequest*>(ptr);
    ep0NextStatusIn_ = req->wLength == 0 || !(req->bmRequestType & SetupRequest::DIRECTION_DEVICE_TO_HOST);
}

void RtlUsbDriver::ep0TransferEvent(bool setup, bool inComplete, bool outComplete) {
    // A control transfer completes with its status stage: a completed data stage alone doesn't count. A SETUP packet
    // completes an OUT transfer too, between its read and its SETUP phase done: that XFRC is not a status stage.
    const bool complete = ep0StatusIn_ ? inComplete : (outComplete && !ep0SetupRead_);
    if (complete && ep0SetupPending_) {
        ep0SetupPending_ = false;
        ep0Completed_++;
    }
    if (setup) {
        ep0SetupPending_ = true;
        ep0SetupRead_ = false;
        ep0StatusIn_ = ep0NextStatusIn_;
    }
}

// EP0 watchdog. Once the host starts a control transfer, the device answers it within milliseconds whatever else the
// bus carries: Particle requests never block at the USB level (a slow one keeps answering CHECK with PENDING), and
// standard and class requests are answered by the USB task. So if the bus is alive and the host keeps waiting on EP0
// (its EP0 tokens are NAKed, or a global IN or OUT NAK stays in effect) for RTL_USB_EP0_WATCHDOG_TIMEOUT without a
// single control transfer completing, EP0 is wedged. Repair EP0 the way the SDK sets it up (RX FIFO flushed, EP0 OUT
// re-armed, global IN and OUT NAK cleared); if control transfers still don't complete, reattach.
void RtlUsbDriver::ep0WatchdogCheck() {
    const system_tick_t now = HAL_Timer_Get_Milli_Seconds();
    // Bus alive: the SOF frame number moved since the previous check
    const uint32_t frame = (USB_DEVICE->DSTS & USB_OTG_DSTS_FNSOF) >> USB_OTG_DSTS_FNSOF_Pos;
    const bool busAlive = frame != ep0LastFrame_;
    ep0LastFrame_ = frame;
    // EP0 tokens NAKed since the previous check: the host is waiting on EP0. Clear (W1C) only the NAK bits.
    const bool inNak = USB_INEP(0)->DIEPINT & USB_OTG_DIEPINT_NAK;
    const bool outNak = USB_OUTEP(0)->DOEPINT & USB_OTG_DOEPINT_NAK;
    if (inNak) {
        USB_INEP(0)->DIEPINT = USB_OTG_DIEPINT_NAK;
    }
    if (outNak) {
        USB_OUTEP(0)->DOEPINT = USB_OTG_DOEPINT_NAK;
    }
    // A global IN or OUT NAK in effect: nothing sets one for longer than a few microseconds, so the core is NAKing every
    // IN or OUT token. The host may not show it on EP0's NAK bits: with one control transfer outstanding, it retries
    // its NAKed packet only every second or so
    const bool globalNak = USB_DEVICE->DCTL & (USB_OTG_DCTL_GINSTS | USB_OTG_DCTL_GONSTS);
    const unsigned completed = ep0Completed_;
    const bool progress = completed != ep0LastCompleted_;
    ep0LastCompleted_ = completed;
    if (!initialized_ || !rtlDev_ || config_ == 0 || needsReset_ || progress || !busAlive ||
            !(inNak || outNak || globalNak)) {
        // Healthy, or nothing to judge: (re)start the wait
        ep0WaitStart_ = now;
        if (progress || config_ == 0) {
            ep0Repaired_ = false;
        }
        return;
    }
    if (now - ep0WaitStart_ < RTL_USB_EP0_WATCHDOG_TIMEOUT) {
        return;
    }
    ep0WaitStart_ = now;
    if (!ep0Repaired_) {
        ep0Repaired_ = true;
        LOG(WARN, "No control transfer completed for %u ms while the host waits on EP0: repairing EP0",
                (unsigned)RTL_USB_EP0_WATCHDOG_TIMEOUT);
        std::lock_guard<RtlUsbDriver> lk(*this);
        SdkLock sdk(pcd());
        repairEp0();
        ep0Repairs_++;
    } else {
        LOG(WARN, "No control transfer completed for %u ms after an EP0 repair: reattaching",
                (unsigned)RTL_USB_EP0_WATCHDOG_TIMEOUT);
        requestReattach();
        ep0Reattaches_++;
    }
}

void RtlUsbDriver::repairEp0() {
    // Nothing in the driver or the SDK sets the global IN NAK, but the core can leave it on, NAKing every IN token
    // Clear it here
    USB_DEVICE->DCTL |= USB_OTG_DCTL_CGINAK;
    // Set global OUT NAK: Stop the core from storing OUT data during the flush. 
    USB_DEVICE->DCTL |= USB_OTG_DCTL_SGONAK;
    usb_hal_flush_rx_fifo();
    // Arm the OUT transfer to expect up to 3 SETUP packets. 
    // STUPCNT = 3, PKTCNT = 1, XFRSIZ = 24 (0x60080018), EPENA left clear, like usbd_hal_ep0_out_start()
    USB_OUTEP(0)->DOEPTSIZ = (RTL_USB_EP0_SETUP_COUNT << USB_OTG_DOEPTSIZ_STUPCNT_Pos) |
            (1UL << USB_OTG_DOEPTSIZ_PKTCNT_Pos) | (RTL_USB_EP0_SETUP_COUNT * sizeof(SetupRequest));
    USB_OUTEP(0)->DOEPINT = 0xffff;
    // Clear global OUT NAK
    USB_DEVICE->DCTL |= USB_OTG_DCTL_CGONAK;
}

// A new SETUP cancels any unfinished control transfer (USB 2.0 8.5.3), but the SDK doesn't cancel EP0 IN: a reply or
// status ZLP the host abandoned stays armed or queued, goes out as the data stage of the next control read, and every
// reply after it is one request late. EP0 shares the TX FIFO and its request queue with CDC IN, so the stale entries
// can only be removed by flushing that FIFO, which cancels the CDC IN transfer in flight as well: its data is dropped,
// and the class is told the transfer completed (see completeCancelledInTransfers()) so it carries on with the next.
// Called by the USB task with the SDK lock held, before the SDK reads an 8-byte packet into ptr.
bool RtlUsbDriver::isSetupBuffer(const void* ptr) const {
    if (!rtlDev_) {
        return false;
    }
    // Otherwise an 8-byte data packet, not a SETUP
    return ptr == *((void**)((uint8_t*)pcd() + RTL_USB_PCD_SETUP_BUF_OFFSET));
}

void RtlUsbDriver::cancelControlTransferOnSetup(void* ptr) {
    if (!(USB_INEP(0)->DIEPCTL & USB_OTG_DIEPCTL_EPENA) || !isSetupBuffer(ptr)) {
        return;
    }
    setupCancels_++;
    // Stop the core taking data out of the shared FIFO while the endpoints are disabled and it is flushed
    USB_DEVICE->DCTL |= USB_OTG_DCTL_SGINAK;
    for (unsigned i = 0; i < RTL_USB_EP_DISABLE_POLLS && !(USB_GLOBAL->GINTSTS & USB_OTG_GINTSTS_GINAKEFF); i++) {
    }
    for (unsigned ep = 0; ep < USBD_MAX_ENDPOINTS; ep++) {
        // Control and bulk endpoints use the shared (non-periodic) FIFO
        const uint32_t type = USB_INEP(ep)->DIEPCTL & USB_OTG_DIEPCTL_EPTYP;
        if ((USB_INEP(ep)->DIEPCTL & USB_OTG_DIEPCTL_EPENA) && (type == 0 || type == USB_OTG_DIEPCTL_EPTYP_1)) {
            abortInTransfer(ep);
        }
    }
    usb_hal_flush_tx_fifo(0x00);
    USB_DEVICE->DCTL |= USB_OTG_DCTL_CGINAK;
}

// Cancel an IN transfer: disable the endpoint and mark the SDK's transfer record done, so the SDK doesn't carry on with
// it (it re-enables the TX FIFO empty interrupt for any IN endpoint with data pending). The caller flushes the FIFO.
// Called with the SDK lock held.
void RtlUsbDriver::abortInTransfer(unsigned ep) {
    auto in = USB_INEP(ep);
    if (in->DIEPCTL & USB_OTG_DIEPCTL_EPENA) {
        in->DIEPCTL |= USB_OTG_DIEPCTL_SNAK | USB_OTG_DIEPCTL_EPDIS;
        for (unsigned i = 0; i < RTL_USB_EP_DISABLE_POLLS && !(in->DIEPINT & USB_OTG_DIEPINT_EPDISD); i++) {
        }
        // Handled here: not for the SDK
        in->DIEPINT = USB_OTG_DIEPINT_EPDISD;
    }
    auto sdkEp = (uint8_t*)pcd() + RTL_USB_PCD_IN_EP_OFFSET + ep * RTL_USB_PCD_EP_SIZE;
    auto len = (volatile uint32_t*)(sdkEp + RTL_USB_PCD_EP_XFER_LEN_OFFSET);
    auto count = (volatile uint32_t*)(sdkEp + RTL_USB_PCD_EP_XFER_COUNT_OFFSET);
    if (*count < *len) {
        *count = *len;
    }
    if (ep) {
        // The class is told the transfer completed from usbd_watch, outside the SDK lock
        cancelledInEps_ |= 1UL << ep;
    }
}

// Called from usbd_watch: tell the classes their IN transfers cancelled by cancelControlTransferOnSetup() completed
void RtlUsbDriver::completeCancelledInTransfers() {
    const uint32_t eps = __atomic_exchange_n(&cancelledInEps_, 0, __ATOMIC_SEQ_CST);
    for (unsigned ep = 1; eps && ep < USBD_MAX_ENDPOINTS; ep++) {
        if (eps & (1UL << ep)) {
            epDataInCb(rtlDev_, ep);
        }
    }
}

// The SDK's TX FIFO empty handler writes data only for endpoints the host has sent IN tokens to, but re-enables the
// interrupt for any IN endpoint with data pending. When the host stops polling CDC IN with a transfer pending (it
// stopped reading, or closed the port while the app was writing), the interrupt fires again at once, forever: the USB
// task takes the CPU and the system thread starves (control requests never complete). On a burst of these with the TX
// FIFO and request queue empty and no EP0 reply pending, cancel the pending bulk IN transfers (see abortInTransfer())
// and mask the interrupt as a safety net; usbd_watch re-enables it about a second later, and the SDK does when it
// starts an IN transfer. Called by the USB task, with or without the SDK lock held; returns true if it masked the
// interrupt.
bool RtlUsbDriver::txFifoEmptyStormCheck() {
    const uint32_t empty = (RTL_USB_NPTX_QUEUE_DEPTH << USB_OTG_GNPTXSTS_NPTQXSAV_Pos) | (USB_GLOBAL->GNPTXFSIZ >> 16);
    if ((USB_GLOBAL->GNPTXSTS & (USB_OTG_GNPTXSTS_NPTQXSAV | USB_OTG_GNPTXSTS_NPTXFSAV)) != empty ||
            (USB_INEP(0)->DIEPCTL & USB_OTG_DIEPCTL_EPENA)) {
        txFifoEmptyPasses_ = 0;
        return false;
    }
    if (++txFifoEmptyPasses_ < RTL_USB_TX_FIFO_EMPTY_STORM_PASSES) {
        return false;
    }
    txFifoEmptyPasses_ = 0;
    // Cancel the pending bulk IN transfers the host isn't collecting now, rather than at the CDC TX timeout (2.5 s):
    // until then the stale transfer keeps the USB task busy and the system thread starves
    if (rtlDev_) {
        for (unsigned ep = 1; ep < USBD_MAX_ENDPOINTS; ep++) {
            if ((USB_INEP(ep)->DIEPCTL & (USB_OTG_DIEPCTL_EPENA | USB_OTG_DIEPCTL_EPTYP)) ==
                    (USB_OTG_DIEPCTL_EPENA | USB_OTG_DIEPCTL_EPTYP_1)) {
                abortInTransfer(ep);
            }
        }
    }
    txFifoEmptyStorms_++;
    USB_GLOBAL->GINTMSK &= ~USB_OTG_GINTMSK_NPTXFEM;
    txFifoEmptyMasked_ = true;
    return true;
}

void RtlUsbDriver::txFifoEmptyRearm() {
    if (!txFifoEmptyMasked_ || !rtlDev_) {
        return;
    }
    std::lock_guard<RtlUsbDriver> lk(*this);
    SdkLock sdk(pcd());
    txFifoEmptyMasked_ = false;
    USB_GLOBAL->GINTMSK |= USB_OTG_GINTMSK_NPTXFEM;
}
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

int RtlUsbDriver::getRecoveryStats(hal_usb_recovery_stats* stats) const {
    hal_usb_recovery_stats s = {};
    s.size = sizeof(s);
    s.ep0_repairs = ep0Repairs_;
    s.ep0_reattaches = ep0Reattaches_;
    s.reattaches = reattaches_;
    s.setup_cancels = setupCancels_;
    s.tx_fifo_empty_storms = txFifoEmptyStorms_;
    const size_t n = std::min<size_t>(stats->size, sizeof(s));
    memcpy(stats, &s, n);
    stats->size = n;
    return 0;
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
    SdkLock sdk(pcd());
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    if ((ep & SetupRequest::DIRECTION_DEVICE_TO_HOST) && (ep & 0x7f)) {
        // CDC TX timeout: the host stopped reading. Flushing alone leaves the endpoint enabled and the SDK's transfer
        // pending, which keeps the TX FIFO empty interrupt firing (see txFifoEmptyStormCheck()).
        abortInTransfer(ep & 0x7f);
    }
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    auto r = usb_hal_flush_tx_fifo(ep & 0x7f);
    // FIXME: USB stack may get into a weird state with EP0 endpoint.
    // Additionally flushing RX fifo seems to resolve the issue.
    usb_hal_flush_tx_fifo(0x00);
    usb_hal_flush_rx_fifo();
    return CHECK_RTL_USB_TO_SYSTEM(r);
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
