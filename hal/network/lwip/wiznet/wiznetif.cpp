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

#include "logging.h"
LOG_SOURCE_CATEGORY("net.en")

#include "wiznetif.h"
#include "wiznetif_config.h"
#include <lwip/opt.h>
#include <lwip/mem.h>
#include <lwip/pbuf.h>
#include <lwip/ethip6.h>
#include <lwip/etharp.h>
#include "delay_hal.h"
#include "timer_hal.h"
#include "gpio_hal.h"
#include "service_debug.h"
#undef SOCK_STREAM
#undef SOCK_DGRAM
#include "wizchip_conf.h"
#include <netif/ethernet.h>
#include <lwip/netifapi.h>
#include <lwip/dhcp.h>
#include <lwip/dns.h>
#include <algorithm>
#include "lwiplock.h"
#include "interrupts_hal.h"
#include "deviceid_hal.h"
#include "bytes2hexbuf.h"
#include "system_error.h"
#include "concurrent_hal.h"

#include "platform_config.h"
#include "spi_lock.h"
#include "check.h"

#ifndef WIZNET_SPI_MODE
#define WIZNET_SPI_MODE SPI_MODE3
#endif /* WIZNET_SPI_MODE */

#ifndef WIZNET_SPI_BITORDER
#define WIZNET_SPI_BITORDER MSBFIRST
#endif /* WIZNET_SPI_BITORDER */

#ifndef WIZNET_SPI_CLOCK
#define WIZNET_SPI_CLOCK (HAL_PLATFORM_ETHERNET_FEATHERWING_SPI_CLOCK)
#endif /* WIZNET_SPI_CLOCK */

#define WAIT_TIMED(timeout, expr) ({                                            \
    uint32_t _millis = HAL_Timer_Get_Milli_Seconds();                           \
    bool res = true;                                                            \
    while((expr))                                                               \
    {                                                                           \
        int32_t dt = (HAL_Timer_Get_Milli_Seconds() - _millis);                 \
        bool nok = ((timeout < dt)                                              \
                   && (expr))                                                   \
                   || (dt < 0);                                                 \
        if (nok)                                                                \
        {                                                                       \
            res = false;                                                        \
            break;                                                              \
        }                                                                       \
        /* I'd really love to get rid of this */                                \
        HAL_Delay_Milliseconds(1);                                              \
    }                                                                           \
    res;                                                                        \
})

using namespace particle::net;

namespace {

// Copy-paste from w5500.c
void wiz_recv_data_ext(uint8_t sn, uint8_t *wizdata, uint16_t len, bool increase = true)
{
    uint16_t ptr = 0;
    uint32_t addrsel = 0;

    if(len == 0) return;
    ptr = getSn_RX_RD(sn);
    //M20140501 : implict type casting -> explict type casting
    //addrsel = ((ptr << 8) + (WIZCHIP_RXBUF_BLOCK(sn) << 3);
    addrsel = ((uint32_t)ptr << 8) + (WIZCHIP_RXBUF_BLOCK(sn) << 3);
    //
    WIZCHIP_READ_BUF(addrsel, wizdata, len);
    ptr += len;

    if (increase) {
        setSn_RX_RD(sn,ptr);
    }
}

const hal_spi_info_t WIZNET_DEFAULT_CONFIG = {
    .version = HAL_SPI_INFO_VERSION,
    .system_clock = 0,
    .default_settings = 0,
    .enabled = true,
    .mode = SPI_MODE_MASTER,
    .clock = WIZNET_SPI_CLOCK,
    .bit_order = WIZNET_SPI_BITORDER,
    .data_mode = WIZNET_SPI_MODE,
    .ss_pin = PIN_INVALID
};

const int WIZNET_DEFAULT_TIMEOUT = 500;
const int WIZNET_DEFAULT_TIMEOUT_POLL = 100;
/* FIXME */
const unsigned int WIZNET_INRECV_NEXT_BACKOFF = 50;
const unsigned int WIZNET_DEFAULT_RX_FRAMES_PER_ITERATION = PBUF_POOL_SIZE / 2;

} /* anonymous */

WizNetif* WizNetif::instance_ = nullptr;

WizNetif::WizNetif(hal_spi_interface_t spi, const uint8_t mac[6])
        : BaseNetif(),
          spi_(spi),
          pwrState_(IF_POWER_STATE_NONE),
          spiLock_(spi, WIZNET_DEFAULT_CONFIG) {

    LOG(INFO, "Creating Wiznet LwIP interface");

    instance_ = this;

    SPARK_ASSERT(os_semaphore_create(&spiSem_, 1, 0) == 0);

    reg_wizchip_cris_cbfunc(
        [](void) -> void {
            auto self = instance();
            self->spiLock_.lock();
        },
        [](void) -> void {
            auto self = instance();
            self->spiLock_.unlock();
        }
    );
    reg_wizchip_cs_cbfunc(
        [](void) -> void {
            auto self = instance();
            hal_gpio_write(self->cs_, 0);
        },
        [](void) -> void {
            auto self = instance();
            hal_gpio_write(self->cs_, 1);
        }
    );
    reg_wizchip_spi_cbfunc(
        [](void) -> uint8_t {
            auto self = instance();
            return hal_spi_transfer(self->spi_, 0xff);
        },
        [](uint8_t wb) -> void {
            auto self = instance();
            hal_spi_transfer(self->spi_, wb);
        }
    );
    reg_wizchip_spiburst_cbfunc(
        [](uint8_t* pBuf, uint16_t len) -> void {
            auto self = instance();
            size_t r = 0;
            while (r < len) {
                /* FIXME: maximum DMA transfer size should be correctly handled by HAL */
                size_t t = std::min((len - r), (size_t)65535);
                hal_spi_transfer_dma(self->spi_, nullptr, pBuf + r, t, [](void) -> void {
                    auto self = instance();
                    os_semaphore_give(self->spiSem_, true);
                });
                auto self = instance();
                os_semaphore_take(self->spiSem_, WIZNET_DEFAULT_TIMEOUT, true);
                r += t;
            }
        },
        [](uint8_t* pBuf, uint16_t len) -> void {
            auto self = instance();
            size_t r = 0;
            while (r < len) {
                /* FIXME: maximum DMA transfer size should be correctly handled by HAL */
                size_t t = std::min((len - r), (size_t)65535);
                hal_spi_transfer_dma(self->spi_, pBuf + r, nullptr, t, [](void) -> void {
                    auto self = instance();
                    os_semaphore_give(self->spiSem_, true);
                });
                auto self = instance();
                os_semaphore_take(self->spiSem_, WIZNET_DEFAULT_TIMEOUT, true);
                r += t;
            }
        }
    );

    netif_.hwaddr_len = ETHARP_HWADDR_LEN;
    memcpy(netif_.hwaddr, mac, ETHARP_HWADDR_LEN);

    exit_ = false;
    down_ = true;
    if (!netifapi_netif_add(interface(), nullptr, nullptr, nullptr, this, initCb, ethernet_input)) {
        SPARK_ASSERT(os_queue_create(&queue_, sizeof(void*), 256, nullptr) == 0);
        registerHandlers();
    }
}

WizNetif::~WizNetif() {
    exit_ = true;
    if (queue_) {
        const void* dummy = nullptr;
        os_queue_put(queue_, &dummy, 1000, nullptr);
    }

    if (thread_) {
        os_thread_join(thread_);
        os_queue_destroy(queue_, nullptr);
    }

    if (spiSem_) {
        os_semaphore_destroy(spiSem_);
    }

    hal_spi_acquire(spi_, nullptr);
    hal_spi_end(spi_);
    hal_spi_release(spi_, nullptr);

    hal_gpio_mode(reset_, INPUT);
    hal_gpio_mode(cs_, INPUT);
    hal_gpio_mode(interrupt_, INPUT);
}

err_t WizNetif::initCb(netif* netif) {
    WizNetif* self = static_cast<WizNetif*>(netif->state);

    return self->initInterface();
}

err_t WizNetif::initInterface() {
    netif_.name[0] = 'e';
    netif_.name[1] = 'n';

    netif_.mtu = 1500;
    netif_.flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;
    /* FIXME: Remove once we enable IPv6 */
    netif_.flags |= NETIF_FLAG_NO_ND6;
    /* netif_.flags |= NETIF_FLAG_MLD6 */

    netif_.output = etharp_output;
#if LWIP_IPV6
    netif_.output_ip6 = ethip6_output;
#endif // LWIP_IPV6
    netif_.linkoutput = &WizNetif::linkOutputCb;

    uint8_t deviceId[HAL_DEVICE_ID_SIZE] = {};
    uint8_t deviceIdLen = hal_get_device_id(deviceId, sizeof(deviceId));
    hostname_ = std::make_unique<char[]>(deviceIdLen * 2 + 1);
    if (hostname_) {
        bytes2hexbuf_lower_case(deviceId, deviceIdLen, hostname_.get());
        hostname_[deviceIdLen * 2] = '\0';
    }
    netif_set_hostname(&netif_, hostname_.get());

    return ERR_OK;
}

void WizNetif::hwReset() {
    if (reset_ != PIN_INVALID) {
        hal_gpio_write(reset_, 0);
        HAL_Delay_Milliseconds(1);
        hal_gpio_write(reset_, 1);
        HAL_Delay_Milliseconds(1);
    }
}

void WizNetif::swReset() {
    wizchip_sw_reset();
}

int WizNetif::init(const WizNetifConfigData& config) {
    if (thread_) {
        // Just in case
        return SYSTEM_ERROR_INVALID_STATE;
    }
    interrupt_ = config.int_pin;
    reset_ = config.reset_pin;
    cs_ = config.cs_pin;

    hal_gpio_config_t conf = {
        .size = sizeof(conf),
        .version = HAL_GPIO_VERSION,
        .mode = OUTPUT,
        .set_value = true,
        .value = 1,
        .drive_strength = HAL_GPIO_DRIVE_DEFAULT
    };
    hal_gpio_configure(reset_, &conf, nullptr);
    hal_gpio_configure(cs_, &conf, nullptr);
    /* There should be an external 10k pull-up, but if there isn't one activate internal one
     * to prevent interrupt line from floating. Worst case it's (13k + 10k) on Gen 3 and (50k + 10k) on Gen 4.
     */
    hal_gpio_mode(interrupt_, INPUT_PULLUP);

    hwReset();
    if (!isPresent(true)) {
        pwrState_ = IF_POWER_STATE_DOWN;
        return SYSTEM_ERROR_NOT_FOUND;
    }
    if (reset_ == PIN_INVALID) {
        swReset();
    }
    pwrState_ = IF_POWER_STATE_UP;

    SPARK_ASSERT(os_thread_create(&thread_, "wiz", OS_THREAD_PRIORITY_NETWORK, &WizNetif::loop, this, OS_THREAD_STACK_SIZE_DEFAULT_HIGH) == 0);

    return 0;
}

bool WizNetif::isPresent(bool retry) {
    const auto RETRY_DELAY = 10;
    // It takes about 250ms for W5500 to startup usually
    const auto MAX_STARTUP_TIME = 300;
    uint8_t retries = retry ? MAX_STARTUP_TIME / RETRY_DELAY : 1;
    uint8_t cv = 0;
    for (uint8_t i = 0; i < retries; i++) {
        cv = getVERSIONR();
        /* VERSIONR always indicates the W5500 version as 0x04 */
        if (cv != 0x04 && retry) {
            HAL_Delay_Milliseconds(RETRY_DELAY);
            continue;
        }
        break;
    }
    if (cv != 0x04) {
        LOG(INFO, "No W5500 present");
        return false;
    }

    auto pwr = pwrState_.load();
    pwrState_ = IF_POWER_STATE_UP;
    if (pwr != IF_POWER_STATE_UP) {
        notifyPowerState(IF_POWER_STATE_UP);
    }

    return true;
}

void WizNetif::interruptCb(void* arg) {
    WizNetif* self = static_cast<WizNetif*>(arg);
    if (self && !self->inRecv_) {
        self->inRecv_ = true;
        const void* dummy = nullptr;
        os_queue_put(self->queue_, &dummy, 0, nullptr);
    }
}

void WizNetif::loop(void* arg) {
    WizNetif* self = static_cast<WizNetif*>(arg);
    unsigned int timeout = WIZNET_DEFAULT_TIMEOUT;
    while(!self->exit_) {
        pbuf* p = nullptr;
        os_queue_take(self->queue_, (void*)&p, timeout, nullptr);
        timeout = (self->interrupt_ == PIN_INVALID) ? WIZNET_DEFAULT_TIMEOUT_POLL : WIZNET_DEFAULT_TIMEOUT;
        if (p) {
            self->output(p);
        }
        if (self->inRecv_ || self->interrupt_ == PIN_INVALID) {
            int r = 1;
            if (!p) {
                r = self->input();
            }
            if (r) {
                timeout = WIZNET_INRECV_NEXT_BACKOFF;
            }
        }
        self->pollState();
    }

    self->down();

    os_thread_exit(nullptr);
}

int WizNetif::up() {
    LwipTcpIpCoreLock lk;

    // We are not yet properly initialized!
    CHECK_TRUE(thread_, SYSTEM_ERROR_INVALID_STATE);

    uint8_t bufSizes[_WIZCHIP_SOCK_NUM_] = {16};
    wizchip_init(bufSizes, bufSizes);

    wiz_NetInfo inf = {};
    memcpy(inf.mac, netif_.hwaddr, netif_.hwaddr_len);

    wizchip_setnetinfo(&inf);

    /* Disable all interrupts */
    setIMR(0x00);
    /* Clear all interrupts */
    setIR(0xff);

    int r = openRaw();
    if (!r) {
        /* Attach interrupt handler */
        hal_interrupt_attach(interrupt_, &WizNetif::interruptCb, this, FALLING, nullptr);
        down_ = false;
    }

    // Just in case. We might call up() directly after powerDown().
    if (pwrState_ != IF_POWER_STATE_UP) {
        notifyPowerState(IF_POWER_STATE_UP);
    }

    return r;
}

int WizNetif::down() {
    LwipTcpIpCoreLock lk;
    down_ = true;
    /* Detach interrupt handler */
    hal_interrupt_detach(interrupt_);

    return closeRaw();
}

int WizNetif::powerUp() {
    // FIXME: As long as the interface is initialized,
    // it must have been powered up as of right now.
    // The system network manager transit the interface to powering up,
    // we should always notify the event to transit the interface to powered up.
    notifyPowerState(IF_POWER_STATE_UP);
    return SYSTEM_ERROR_NONE;
}

int WizNetif::powerDown() {
    int ret = down();
    // FIXME: This don't really power off the module.
    // Notify the system network manager that the module is powered down
    // to bypass waitInterfaceOff() as required by system sleep.
    // The system network manager transit the interface to powering down,
    // we should always notify the event to transit the interface to powered down.
    notifyPowerState(IF_POWER_STATE_DOWN);
    return ret;
}

void WizNetif::notifyPowerState(if_power_state_t state) {
    pwrState_ = state;
    if_event evt = {};
    struct if_event_power_state ev_if_power_state = {};
    evt.ev_len = sizeof(if_event);
    evt.ev_type = IF_EVENT_POWER_STATE;
    evt.ev_power_state = &ev_if_power_state;
    evt.ev_power_state->state = pwrState_;
    if_notify_event(interface(), &evt, nullptr);
}

int WizNetif::getPowerState(if_power_state_t* state) const {
    *state = pwrState_;
    return SYSTEM_ERROR_NONE;
}

int WizNetif::getNcpState(unsigned int* state) const {
    // TODO: implement it
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int WizNetif::openRaw() {
    LwipTcpIpCoreLock lk;

    closeRaw();

    // Use maximum buffer sizes for MACRAW socket 0
    // This should happen in wizchip_init() but just in case doing it again
    setSn_RXBUF_SIZE(0, 16);
    setSn_TXBUF_SIZE(0, 16);
    /* Set mode to MACRAW, enable MAC filtering and IPv6 packet filtering (not supported right now anyway) */
    setSn_MR(0, Sn_MR_MACRAW | Sn_MR_MFEN | Sn_MR_MIP6B);
    /* Set interrupt mask to input only */
    setSn_IMR(0, Sn_IR_RECV);
    /* Enable interrupts on socket 0 */
    setSIMR(0x01);
    /* Open socket */
    setSn_CR(0, Sn_CR_OPEN);
    /* After W5500 accepts the command, the Sn_CR register is automatically cleared to 0x00. */
    WAIT_TIMED(WIZNET_DEFAULT_TIMEOUT, getSn_CR(0));
    /* Waiting for it to get opened */
    WAIT_TIMED(WIZNET_DEFAULT_TIMEOUT, getSn_SR(0) == SOCK_CLOSED);

    int st = !(getSn_SR(0) == SOCK_MACRAW);

    LOG(TRACE, "Opened MACRAW socket, err = %d", st);

    return st;
}

int WizNetif::closeRaw() {
    LwipTcpIpCoreLock lk;

    setSn_CR(0, Sn_CR_CLOSE);
    /* After W5500 accepts the command, the Sn_CR register is automatically cleared to 0x00. */
    WAIT_TIMED(WIZNET_DEFAULT_TIMEOUT, getSn_CR(0));
    /* Clear interrupts */
    setSn_IR(0, 0xff);
    /* Disable all interrupts */
    setSIMR(0x00);
    /* Waiting for it to get closed */
    WAIT_TIMED(WIZNET_DEFAULT_TIMEOUT, getSn_SR(0) != SOCK_CLOSED);

    LOG(TRACE, "Closed MACRAW socket");

    netif_set_link_down(interface());

    return 0;
}

void WizNetif::pollState() {
    if (!netif_is_up(interface()) || down_) {
        return;
    }

    if (HAL_Timer_Get_Milli_Seconds() - lastStatePoll_ < 500) {
        return;
    }

    /* Poll link state and update if necessary */
    auto linkState = wizphy_getphylink() == PHY_LINK_ON;

    LwipTcpIpCoreLock lk;
    if (netif_is_link_up(&netif_) != linkState) {
        if (linkState) {
            LOG(INFO, "Link up");
            netif_set_link_up(&netif_);
        } else {
            LOG(INFO, "Link down");
            netif_set_link_down(&netif_);
        }
    }

    lastStatePoll_ = HAL_Timer_Get_Milli_Seconds();
}

int WizNetif::input() {
    if (down_) {
        return 0;
    }

    uint16_t size = 0;
    unsigned int count = 0;

    int r = 0;

    while ((size = getSn_RX_RSR(0)) > 0 && size != 0xffff && count < WIZNET_DEFAULT_RX_FRAMES_PER_ITERATION) {
        uint16_t pktSize = 0;
        {
            uint8_t tmp[2] = {};
            wiz_recv_data_ext(0, tmp, sizeof(tmp), false /* do not increase rx pointer */);
            setSn_CR(0, Sn_CR_RECV);
            WAIT_TIMED(WIZNET_DEFAULT_TIMEOUT, getSn_CR(0));
            pktSize = (tmp[0] << 8 | tmp[1]) - 2;
        }

        if (pktSize >= 1522 /* IEEE 802.3ac max size */) {
            // Attempt to recover
            LOG_DEBUG(WARN, "invalid packet size %u, attempting to recover the RX buffer", pktSize);
            wiz_recv_ignore(0, pktSize + 2);
            closeRaw();
            openRaw();
            break;
        }

#if ETH_PAD_SIZE
        /* allow room for Ethernet padding */
        pktSize += ETH_PAD_SIZE;
#endif /* ETH_PAD_SIZE */

        pbuf* p = pbuf_alloc(PBUF_RAW, pktSize, PBUF_POOL);
        if (p != nullptr) {
#if ETH_PAD_SIZE
            /* drop the padding word */
            pbuf_remove_header(p, ETH_PAD_SIZE);
#endif /* ETH_PAD_SIZE */
            wiz_recv_ignore(0, 2); // Skip size
            setSn_CR(0, Sn_CR_RECV);
            WAIT_TIMED(WIZNET_DEFAULT_TIMEOUT, getSn_CR(0));
            for (pbuf* q = p; q != nullptr; q = q->next) {
                wiz_recv_data(0, (uint8_t*)q->payload, q->len);
                setSn_CR(0, Sn_CR_RECV);
                WAIT_TIMED(WIZNET_DEFAULT_TIMEOUT, getSn_CR(0));
            }
#if ETH_PAD_SIZE
            /* reclaim the padding word */
            pbuf_add_header(p, ETH_PAD_SIZE);
#endif /* ETH_PAD_SIZE */
            {
                LwipTcpIpCoreLock lk;
                if (netif_.input(p, &netif_) != ERR_OK) {
                    LOG(ERROR, "Error inputing packet");
                    pbuf_free(p);
                }
            }
        } else {
            LOG(ERROR, "Failed to allocate pbuf");
#if ETH_PAD_SIZE
            pktSize -= ETH_PAD_SIZE;
#endif /* ETH_PAD_SIZE */
   
            /* Giving a chance to free up some pbufs */
            /* NOT dropping packet here, keeping it inside W5500 RAM */
            r = 1;
            break;
        }

        ++count;
    }

    auto rxr = getSn_RX_RSR(0);

    if (rxr == 0 || rxr == 0xffff) {
        inRecv_ = false;
        setSn_IR(0, Sn_IR_RECV);
    } else {
        // back-off
        r = 1;
    }

    return r;
}

err_t WizNetif::linkOutput(pbuf* p) {
    if (!(netif_is_up(interface()) && netif_is_link_up(interface()))) {
        return ERR_RTE;
    }

    pbuf* q = pbuf_clone(PBUF_LINK, PBUF_RAM, p);
    if (!q) {
        LOG(ERROR, "no memory to clone pbuf");
        return ERR_MEM;
    }

    if (os_queue_put(queue_, &q, 0, nullptr)) {
        LOG_DEBUG(ERROR, "Dropping packet %x, not enough space in event queue", q);
        pbuf_free(q);
        return ERR_MEM;
    }
    return ERR_OK;
}

void WizNetif::output(pbuf* p) {
    uint16_t txAvailable = 0;
    uint16_t ptr;

    if (down_) {
        goto cleanup;
    }

#if ETH_PAD_SIZE
    pbuf_remove_header(p, ETH_PAD_SIZE); /* drop the padding word */
#endif

    WAIT_TIMED(WIZNET_DEFAULT_TIMEOUT, (((txAvailable = getSn_TX_FSR(0)) < p->tot_len) && txAvailable != 0xffff));

    if (p->tot_len > txAvailable || txAvailable == 0xffff) {
        /* Drop packet */
        LOG(ERROR, "Dropping packet, not enough space in TX buffer %u", txAvailable);
        // FIXME: this should normally not happen, attempt to recover
        setSn_TX_WR(0, 0);
        setSn_CR(0, Sn_CR_SEND);
        if (getSn_TX_FSR(0) < p->tot_len) {
            down();
            up();
        }
        goto cleanup;
    }

#if HAL_PLATFORM_SPI_DMA_SOURCE_RAM_ONLY
    // For platforms that require the DMA source address to be in RAM (as opposed to in ROM/flash)
    // we need to copy the whole pbuf queue into a single PBUF_RAM
    {
        bool copyToRam = false;
        for (pbuf* q = p; q != nullptr; q = q->next) {
            if (pbuf_match_type(q, PBUF_ROM)) {
                copyToRam = true;
                break;
            }
        }
        if (copyToRam) {
            auto pRam = pbuf_clone(PBUF_RAW, PBUF_RAM, p);
            if (!pRam) {
                LOG_DEBUG(ERROR, "Dropping packet, cannot copy to RAM");
                goto cleanup;
            }
            pbuf_free(p);
            p = pRam;
        }
    }
#endif // HAL_PLATFORM_SPI_DMA_SOURCE_RAM_ONLY

    ptr = getSn_TX_WR(0);

    for (pbuf* q = p; q != nullptr; q = q->next) {
        uint32_t addr = ((uint32_t)ptr << 8) + (WIZCHIP_TXBUF_BLOCK(0) << 3);
        WIZCHIP_WRITE_BUF(addr, (uint8_t*)q->payload, q->len);
        ptr += q->len;
    }

    setSn_TX_WR(0, ptr);

    setSn_CR(0, Sn_CR_SEND);
    /* After W5500 accepts the command, the Sn_CR register is automatically cleared to 0x00. */
    WAIT_TIMED(WIZNET_DEFAULT_TIMEOUT, getSn_CR(0));
    WAIT_TIMED(WIZNET_DEFAULT_TIMEOUT, ((getSn_IR(0) & (Sn_IR_SENDOK | Sn_IR_TIMEOUT)) == 0));
    setSn_IR(0, (Sn_IR_SENDOK | Sn_IR_TIMEOUT));

#if ETH_PAD_SIZE
    pbuf_add_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

cleanup:
    pbuf_free(p);
}

err_t WizNetif::linkOutputCb(netif* netif, pbuf* p) {
    WizNetif* self = static_cast<WizNetif*>(netif->state);

    return self->linkOutput(p);
}

void WizNetif::ifEventHandler(const if_event* ev) {
    if (ev->ev_type == IF_EVENT_STATE) {
        if (ev->ev_if_state->state) {
            up();
        } else {
            down();
        }
    }
}

void WizNetif::netifEventHandler(netif_nsc_reason_t reason, const netif_ext_callback_args_t* args) {
    /* Nothing to do here */
}

int WizNetif::request(if_req_driver_specific* req, size_t size) {
    return WizNetifConfig::instance()->request(req, size);
}
