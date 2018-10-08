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

#ifndef LWIP_ESP32_NCP_NETIF_H
#define LWIP_ESP32_NCP_NETIF_H

#include "basenetif.h"
#include "concurrent_hal.h"
#include "atclient.h"
#include "static_recursive_mutex.h"
#include <atomic>
#include <lwip/netif.h>
#include <lwip/pbuf.h>
#include "gsm0710muxer/muxer.h"

#ifdef __cplusplus

namespace particle { namespace net {

class Esp32NcpNetif : public BaseNetif {
public:
    Esp32NcpNetif(particle::services::at::ArgonNcpAtClient* atclient);
    virtual ~Esp32NcpNetif();

protected:
    virtual void ifEventHandler(const if_event* ev) override;
    virtual void netifEventHandler(netif_nsc_reason_t reason, const netif_ext_callback_args_t* args) override;

private:
    int up();
    int down();
    int upImpl();
    int downImpl();

    /* LwIP netif init callback */
    static err_t initCb(netif *netif);
    err_t initInterface();

    void hwReset();

    static void loop(void* arg);

    static int channelDataHandlerCb(const uint8_t* data, size_t size, void* ctx);

    /* LwIP netif linkoutput callback */
    static err_t linkOutputCb(netif* netif, pbuf* p);
    err_t linkOutput(pbuf* p);

private:
    os_thread_t thread_ = nullptr;
    os_queue_t queue_ = nullptr;
    std::atomic_bool exit_;
    std::atomic_bool start_;
    std::atomic_bool stop_;

    particle::services::at::ArgonNcpAtClient* atClient_ = nullptr;
    particle::Stream* origStream_ = nullptr;
    gsm0710::Muxer<particle::Stream, StaticRecursiveMutex> muxer_;
};

} } // namespace particle::net

#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWIP_ESP32_NCP_NETIF_H */
