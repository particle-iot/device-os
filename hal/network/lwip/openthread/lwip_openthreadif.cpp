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

#include "openthread-core-config.h"
#include "lwip_openthreadif.h"
#include "logging.h"
#include <openthread/instance.h>
#include <lwip/netifapi.h>
#include <lwip/tcpip.h>
#include <lwip/pbuf.h>
#include <lwip/mld6.h>
#include <openthread/ip6.h>
#include <openthread/thread.h>
#include <algorithm>
#include "service_debug.h"

using namespace particle::net;

namespace {

void otNetifAddressToIp6Addr(const otNetifAddress* otAddr, ip6_addr_t& addr) {
  IP6_ADDR(&addr, otAddr->mAddress.mFields.m32[0], otAddr->mAddress.mFields.m32[1], otAddr->mAddress.mFields.m32[2], otAddr->mAddress.mFields.m32[3]);
}

void otNetifMulticastAddressToIp6Addr(const otNetifMulticastAddress* otAddr, ip6_addr_t& addr) {
  IP6_ADDR(&addr, otAddr->mAddress.mFields.m32[0], otAddr->mAddress.mFields.m32[1], otAddr->mAddress.mFields.m32[2], otAddr->mAddress.mFields.m32[3]);
}

int otNetifAddressStateToIp6AddrState(const otNetifAddress* addr) {
  return addr->mValid ? (addr->mPreferred ? IP6_ADDR_PREFERRED : IP6_ADDR_DEPRECATED) : IP6_ADDR_INVALID;
}

} /* anonymous */

OpenThreadNetif::OpenThreadNetif(otInstance* ot)
    : ot_(ot) {

  if (!ot_) {
#if !defined(OPENTHREAD_ENABLE_MULTIPLE_INSTANCES) || OPENTHREAD_ENABLE_MULTIPLE_INSTANCES == 0
    ot_ = otInstanceInitSingle();
#else
    uint8_t* buf = nullptr;
    size_t len = 0;
    otInstanceInit(nullptr, &len);
    buf = (uint8_t*)calloc(1, len);
    SPARK_ASSERT(buf != nullptr);

    ot_ = otInstanceInit(buf, &len);
    SPARK_ASSERT(ot_);
#endif /* !defined(OPENTHREAD_ENABLE_MULTIPLE_INSTANCES) || OPENTHREAD_ENABLE_MULTIPLE_INSTANCES == 0 */
  }


  LOG(INFO, "Creating new LwIP OpenThread interface");
  netifapi_netif_add(interface(), nullptr, nullptr, nullptr, nullptr, initCb, tcpip_input);
  interface()->state = this;
  /* Automatically set it up */
  netifapi_netif_set_up(interface());

  /* Register OpenThread receive callback */
  otIp6SetReceiveCallback(ot_, otReceiveCb, this);
  otIp6SetReceiveFilterEnabled(ot_, false);

  /* Register OpenThread state changed callback */
  otSetStateChangedCallback(ot_, otStateChangedCb, this);
}

OpenThreadNetif::~OpenThreadNetif() {
  /* Unregister OpenThread state changed callback */
  otRemoveStateChangeCallback(ot_, otStateChangedCb, this);
  netifapi_netif_remove(interface());
}

/* LwIP netif init callback */
err_t OpenThreadNetif::initCb(netif* netif) {
  netif->name[0] = 'o';
  netif->name[1] = 't';

  netif->mtu = 1280;
  netif->output_ip6 = outputIp6Cb;
  netif->rs_count = 0;
  netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_NO_ND6;

  return ERR_OK;
}

/* LwIP netif output_ip6 callback */
err_t OpenThreadNetif::outputIp6Cb(netif* netif, pbuf* p, const ip6_addr_t* addr) {
  auto self = static_cast<OpenThreadNetif*>(netif->state);
  // LOG(TRACE, "OpenThreadNetif(%x) output() %lu bytes", self, p->tot_len);
  auto msg = otIp6NewMessage(self->ot_, true);
  if (msg == nullptr) {
    LOG(TRACE, "out of memory");
    return ERR_MEM;
  }

  int ret = OT_ERROR_NONE;

  for (auto q = p; q != nullptr && ret == OT_ERROR_NONE; q = q->next) {
    ret = otMessageAppend(msg, q->payload, q->len);
  }

  if (ret == OT_ERROR_NONE) {
    /* otIp6Send takes ownership of msg */
    // LOG(TRACE, "OpenThreadNetif(%x) sending %lu bytes", self, p->tot_len);
    ret = otIp6Send(self->ot_, msg);
  } else {
    otMessageFree(msg);
  }

  return ret == OT_ERROR_NONE ? ERR_OK : ERR_VAL;
}

/* OpenThread receive callback */
void OpenThreadNetif::otReceiveCb(otMessage* msg, void* ctx) {
  auto self = static_cast<OpenThreadNetif*>(ctx);

  // LOG(TRACE, "otReceiveCb: new message %x", msg);

  self->input(msg);

  otMessageFree(msg);
}

/* OpenThread state changed callback */
void OpenThreadNetif::otStateChangedCb(uint32_t flags, void* ctx) {
  auto self = static_cast<OpenThreadNetif*>(ctx);

  LOG(TRACE, "OpenThread state changed: %x", flags);

  self->stateChanged(flags);
}

void OpenThreadNetif::stateChanged(uint32_t flags) {
  LOCK_TCPIP_CORE();
  /* link state */
  if (flags & OT_CHANGED_THREAD_ROLE) {
    switch (otThreadGetDeviceRole(ot_)) {
      case OT_DEVICE_ROLE_DISABLED:
      case OT_DEVICE_ROLE_DETACHED: {
        netif_set_link_down(interface());
        break;
      }
      default: {
        netif_set_link_up(interface());
      }
    }
  }

  /* IPv6-addresses */
  if (flags & (OT_CHANGED_IP6_ADDRESS_ADDED |
               OT_CHANGED_IP6_ADDRESS_REMOVED |
               OT_CHANGED_THREAD_LL_ADDR |
               OT_CHANGED_THREAD_ML_ADDR)) {
    /* Synchronize IP-addresses */

    uint8_t handledLwip[LWIP_IPV6_NUM_ADDRESSES] = {};
    uint8_t handledOt[LWIP_IPV6_NUM_ADDRESSES] = {};

    /* Check existing addresses and adjust state if necessary */
    int otIdx = 0;
    for (const auto* addr = otIp6GetUnicastAddresses(ot_); addr; addr = addr->mNext) {
      ip6_addr_t ip6addr = {};
      otNetifAddressToIp6Addr(addr, ip6addr);
      ip6_addr_assign_zone(&ip6addr, IP6_UNICAST, interface());
      const auto state = otNetifAddressStateToIp6AddrState(addr);

      int idx = netif_get_ip6_addr_match(interface(), &ip6addr);
      if (idx != -1) {
        /* Address is present */
        if (state != netif_ip6_addr_state(interface(), idx)) {
          /* State needs to be adjusted */
          netif_ip6_addr_set_state(interface(), idx, state);
        }
        handledLwip[idx] = 1;
        handledOt[otIdx] = 1;
      }

      otIdx++;
    }

    /* Clear any other addresses */
    for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
      if (!handledLwip[i]) {
        if ((netif_ip6_addr_state(interface(), i) != IP6_ADDR_INVALID) ||
            !ip6_addr_isany(netif_ip6_addr(interface(), i))) {
          netif_ip6_addr_set_state(interface(), i, IP6_ADDR_INVALID);
          netif_ip6_addr_set_parts(interface(), i, 0, 0, 0, 0);
        }
      }
    }

    /* Add new addresses */
    otIdx = 0;
    for (const auto* addr = otIp6GetUnicastAddresses(ot_); addr; addr = addr->mNext) {
      if (!handledOt[otIdx]) {
        /* New address */
        ip6_addr_t ip6addr = {};
        otNetifAddressToIp6Addr(addr, ip6addr);
        ip6_addr_assign_zone(&ip6addr, IP6_UNICAST, interface());
        const auto state = otNetifAddressStateToIp6AddrState(addr);

        int8_t idx = -1;
        netif_add_ip6_address(interface(), &ip6addr, &idx);
        if (idx >= 0) {
          netif_ip6_addr_set_state(interface(), idx, state);
        }
        char tmp[IP6ADDR_STRLEN_MAX] = {0};
        ip6addr_ntoa_r(&ip6addr, tmp, sizeof(tmp));
        LOG(TRACE, "Added %s", tmp);
      }
      otIdx++;
    }
  }

  if (flags & (OT_CHANGED_IP6_MULTICAST_SUBSRCRIBED | OT_CHANGED_IP6_MULTICAST_UNSUBSRCRIBED)) {
    /* Sychronize multicast groups */
    /* FIXME: naive */
    /* Clear all groups */
    mld6_stop(interface());
    for (const auto* addr = otIp6GetMulticastAddresses(ot_); addr; addr = addr->mNext) {
      ip6_addr_t ip6addr = {};
      otNetifMulticastAddressToIp6Addr(addr, ip6addr);
      mld6_joingroup_netif(interface(), &ip6addr);
    }
  }

  UNLOCK_TCPIP_CORE();

  if (flags & OT_CHANGED_IP6_ADDRESS_ADDED) {
    LOG(TRACE, "OT_CHANGED_IP6_ADDRESS_ADDED");
  }
  if (flags & OT_CHANGED_IP6_ADDRESS_REMOVED) {
    LOG(TRACE, "OT_CHANGED_IP6_ADDRESS_REMOVED");
  }
  if (flags & OT_CHANGED_THREAD_ROLE) {
    LOG(TRACE, "OT_CHANGED_THREAD_ROLE");
  }
  if (flags & OT_CHANGED_THREAD_LL_ADDR) {
    LOG(TRACE, "OT_CHANGED_THREAD_LL_ADDR");
  }
  if (flags & OT_CHANGED_THREAD_ML_ADDR) {
    LOG(TRACE, "OT_CHANGED_THREAD_ML_ADDR");
  }
  if (flags & OT_CHANGED_THREAD_RLOC_ADDED) {
    LOG(TRACE, "OT_CHANGED_THREAD_RLOC_ADDED");
  }
  if (flags & OT_CHANGED_THREAD_RLOC_REMOVED) {
    LOG(TRACE, "OT_CHANGED_THREAD_RLOC_REMOVED");
  }
  if (flags & OT_CHANGED_THREAD_PARTITION_ID) {
    LOG(TRACE, "OT_CHANGED_THREAD_PARTITION_ID");
  }
  if (flags & OT_CHANGED_THREAD_KEY_SEQUENCE_COUNTER) {
    LOG(TRACE, "OT_CHANGED_THREAD_KEY_SEQUENCE_COUNTER");
  }
  if (flags & OT_CHANGED_THREAD_NETDATA) {
    LOG(TRACE, "OT_CHANGED_THREAD_NETDATA");
  }
  if (flags & OT_CHANGED_THREAD_CHILD_ADDED) {
    LOG(TRACE, "OT_CHANGED_THREAD_CHILD_ADDED");
  }
  if (flags & OT_CHANGED_THREAD_CHILD_REMOVED) {
    LOG(TRACE, "OT_CHANGED_THREAD_CHILD_REMOVED");
  }
  if (flags & OT_CHANGED_IP6_MULTICAST_SUBSRCRIBED) {
    LOG(TRACE, "OT_CHANGED_IP6_MULTICAST_SUBSRCRIBED");
  }
  if (flags & OT_CHANGED_IP6_MULTICAST_UNSUBSRCRIBED) {
    LOG(TRACE, "OT_CHANGED_IP6_MULTICAST_UNSUBSRCRIBED");
  }
  if (flags & OT_CHANGED_COMMISSIONER_STATE) {
    LOG(TRACE, "OT_CHANGED_COMMISSIONER_STATE");
  }
  if (flags & OT_CHANGED_JOINER_STATE) {
    LOG(TRACE, "OT_CHANGED_JOINER_STATE");
  }
  if (flags & OT_CHANGED_THREAD_CHANNEL) {
    LOG(TRACE, "OT_CHANGED_THREAD_CHANNEL");
  }
  if (flags & OT_CHANGED_THREAD_PANID) {
    LOG(TRACE, "OT_CHANGED_THREAD_PANID");
  }
  if (flags & OT_CHANGED_THREAD_NETWORK_NAME) {
    LOG(TRACE, "OT_CHANGED_THREAD_NETWORK_NAME");
  }
  if (flags & OT_CHANGED_THREAD_EXT_PANID) {
    LOG(TRACE, "OT_CHANGED_THREAD_EXT_PANID");
  }
  if (flags & OT_CHANGED_MASTER_KEY) {
    LOG(TRACE, "OT_CHANGED_MASTER_KEY");
  }
  if (flags & OT_CHANGED_PSKC) {
    LOG(TRACE, "OT_CHANGED_PSKC");
  }
  if (flags & OT_CHANGED_SECURITY_POLICY) {
    LOG(TRACE, "OT_CHANGED_SECURITY_POLICY");
  }
}

void OpenThreadNetif::input(otMessage* msg) {
  if (!(netif_is_up(interface()) && netif_is_link_up(interface()))) {
    return;
  }
  uint16_t len = otMessageGetLength(msg);
  // LOG(TRACE, "OpenThreadNetif(%x): input() length %u", this, len);
  auto p = pbuf_alloc(PBUF_IP, len, PBUF_POOL);
  if (p) {
    uint16_t written = 0;
    for (auto q = p; q != nullptr && written < len; q = q->next) {
      uint16_t toCopy = std::min((uint16_t)(len - written), q->len);
      written += otMessageRead(msg, written, q->payload, toCopy);
    }
    err_t ret = interface()->input(p, interface());
    if (ret != ERR_OK) {
      LOG(TRACE, "input failed: %x", ret);
      pbuf_free(p);
    }
  } else {
    LOG(TRACE, "input failed to alloc");
  }
}

netif* OpenThreadNetif::interface() {
  return &netif_;
}

otInstance* OpenThreadNetif::getOtInstance() {
  return ot_;
}
