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

#ifndef LWIP_WIZNET_IF_H
#define LWIP_WIZNET_IF_H

#include "basenetif.h"
#include "interrupts_hal.h"
#include "spi_hal.h"
#include "concurrent_hal.h"
#include <atomic>
#include <lwip/netif.h>
#include <lwip/pbuf.h>
#include <memory>

#ifdef __cplusplus

namespace particle { namespace net {

class WizNetif : public BaseNetif {
public:
    WizNetif();
    WizNetif(HAL_SPI_Interface spi, pin_t cs, pin_t reset, pin_t interrupt, const uint8_t mac[6]);
    virtual ~WizNetif();

    virtual int powerUp() override;
    virtual int powerDown() override;

    static WizNetif* instance() {
        return instance_;
    }

protected:
    virtual void ifEventHandler(const if_event* ev) override;
    virtual void netifEventHandler(netif_nsc_reason_t reason, const netif_ext_callback_args_t* args) override;

private:
    /* LwIP netif init callback */
    static err_t initCb(netif *netif);
    err_t initInterface();

    void hwReset();
    bool isPresent();

    static void interruptCb(void* arg);
    static void loop(void* arg);

    int up();
    int down();

    int openRaw();
    int closeRaw();

    void pollState();
    int input();
    void output(pbuf* p);
    /* LwIP netif linkoutput callback */
    static err_t linkOutputCb(netif* netif, pbuf* p);
    err_t linkOutput(pbuf* p);

private:
    HAL_SPI_Interface spi_;
    pin_t cs_;
    pin_t reset_;
    pin_t interrupt_;

    os_thread_t thread_ = nullptr;
    os_queue_t queue_ = nullptr;
    os_semaphore_t spiSem_ = nullptr;

    std::atomic_bool exit_;
    std::atomic_bool inRecv_;
    std::atomic_bool down_;

    system_tick_t lastStatePoll_ = 0;

    /* FIXME: Wiznet callbacks do not have any kind of state arguments :( */
    static WizNetif* instance_;

    std::unique_ptr<char[]> hostname_;
};

} } // namespace particle::net

#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWIP_WIZNET_IF_H */
