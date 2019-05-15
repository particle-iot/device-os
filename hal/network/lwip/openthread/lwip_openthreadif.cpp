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
LOG_SOURCE_CATEGORY("net.th")

#include "openthread-core-config.h"
#include "lwip_openthreadif.h"
#include <openthread/instance.h>
#include <lwip/netifapi.h>
#include <lwip/tcpip.h>
#include <lwip/pbuf.h>
#include <lwip/mld6.h>
#include <openthread/ip6.h>
#include <openthread/thread.h>
#include <algorithm>
#include "service_debug.h"
#include "ot_api.h"
#include <mutex>
#include <lwip/dns.h>
#include <openthread/netdata.h>
#include "ipaddr_util.h"
#include "lwiplock.h"

#include <lwip/opt.h>
#include "hal_platform.h"

// FIXME:
#include "system_threading.h"
extern "C" int system_cloud_set_inet_family_keepalive(int af, unsigned int value, int flags);

using namespace particle::net;

namespace {

void updateIp6CloudKeepalive(unsigned int value) {
    struct Task: public ISRTaskQueue::Task {
        unsigned int value;
    };
    const auto task = new(std::nothrow) Task;
    if (!task) {
        return;
    }
    task->value = value;
    task->func = [](ISRTaskQueue::Task* task) {
        unsigned int value = ((Task*)task)->value;
        delete task;
        system_cloud_set_inet_family_keepalive(AF_INET6, value, 0);
    };
    SystemISRTaskQueue.enqueue(task);
}

void otIp6AddressToIp6Addr(const otIp6Address* otAddr, ip6_addr_t& addr) {
    IP6_ADDR(&addr, otAddr->mFields.m32[0],
                    otAddr->mFields.m32[1],
                    otAddr->mFields.m32[2],
                    otAddr->mFields.m32[3]);
}

void ip6AddrToOtIp6Address(const ip6_addr_t& addr, otIp6Address* otAddr) {
    memcpy(otAddr->mFields.m32, addr.addr, sizeof(addr.addr));
}

void otNetifAddressToIp6Addr(const otNetifAddress* otAddr, ip6_addr_t& addr) {
    IP6_ADDR(&addr, otAddr->mAddress.mFields.m32[0],
                    otAddr->mAddress.mFields.m32[1],
                    otAddr->mAddress.mFields.m32[2],
                    otAddr->mAddress.mFields.m32[3]);
}

void otNetifMulticastAddressToIp6Addr(const otNetifMulticastAddress* otAddr, ip6_addr_t& addr) {
    IP6_ADDR(&addr, otAddr->mAddress.mFields.m32[0],
                    otAddr->mAddress.mFields.m32[1],
                    otAddr->mAddress.mFields.m32[2],
                    otAddr->mAddress.mFields.m32[3]);
}

bool otNetifAddressIsRlocOrAloc(const otNetifAddress* addr) {
    if (addr->mRloc) {
        return true;
    }
    static const auto kAloc16Mask = 0xfc;
    static const auto kRloc16ReservedBitMask = 0x02;
    return (addr->mAddress.mFields.m16[4] == PP_HTONL(0x0000) && addr->mAddress.mFields.m16[5] == PP_HTONL(0x00ff) &&
            addr->mAddress.mFields.m16[6] == PP_HTONL(0xfe00) &&
            ((addr->mAddress.mFields.m8[14] < kAloc16Mask &&
            (addr->mAddress.mFields.m8[14] & kRloc16ReservedBitMask) == 0) ||
            (addr->mAddress.mFields.m8[14] == kAloc16Mask)));
}

int otNetifAddressStateToIp6AddrState(const otNetifAddress* addr) {
    return addr->mValid ? (addr->mPreferred ? IP6_ADDR_PREFERRED : IP6_ADDR_DEPRECATED) : IP6_ADDR_INVALID;
}

enum OtNetifAddressType {
    OT_NETIF_ADDRESS_TYPE_NONE = 0x00,
    OT_NETIF_ADDRESS_TYPE_SLAAC,
    OT_NETIF_ADDRESS_TYPE_DHCP,

    OT_NETIF_ADDRESS_TYPE_ANY = 0xff
};

bool otIp6PrefixEquals(const otIp6Prefix& p1, const otIp6Prefix& p2) {
    if (p1.mLength == p2.mLength && otIp6PrefixMatch(&p1.mPrefix, &p2.mPrefix) >= p1.mLength) {
        return true;
    }

    return false;
}

bool otNetifAddressMatchesPrefix(const otNetifAddress& addr, const otIp6Prefix& pref) {
    if (otIp6PrefixMatch(&pref.mPrefix, &addr.mAddress) >= pref.mLength &&
        pref.mLength == addr.mPrefixLength) {
        return true;
    }

    return false;
}

bool otNetifAddressInNetDataPrefixList(otInstance* ot, const otNetifAddress& addr, OtNetifAddressType type = OT_NETIF_ADDRESS_TYPE_ANY) {
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otBorderRouterConfig config = {};

    while (otNetDataGetNextOnMeshPrefix(ot, &iterator, &config) == OT_ERROR_NONE) {
        switch (type) {
            case OT_NETIF_ADDRESS_TYPE_SLAAC: {
                if (!config.mSlaac) {
                    continue;
                }
                break;
            }
            case OT_NETIF_ADDRESS_TYPE_DHCP: {
                if (!config.mDhcp) {
                    continue;
                }
                break;
            }
            case OT_NETIF_ADDRESS_TYPE_ANY: {
                if (!(config.mSlaac || config.mDhcp)) {
                    continue;
                }
                break;
            }
        }

        if (otNetifAddressMatchesPrefix(addr, config.mPrefix)) {
            return true;
        }
    }

    return false;
}

const char* routerPreferenceToString(int pref) {
    static const char* prefs[] = {
        "Reserved",
        "Low",
        "Medium",
        "High",
    };

    pref += 2;
    if (pref > 3) {
        return nullptr;
    }

    return prefs[pref];
}

const uint8_t THREAD_DNS_SERVER_INDEX =
    std::min(DNS_MAX_SERVERS - 1, std::max(LWIP_DHCP_MAX_DNS_SERVERS, LWIP_DHCP6_MAX_DNS_SERVERS));

} /* anonymous */

OpenThreadNetif::OpenThreadNetif(otInstance* ot)
        : BaseNetif(),
          ot_(ot) {

    ot::ThreadLock lk;
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
    registerHandlers();
    /* Automatically set it up */
    /* netifapi_netif_set_up(interface()); */

    /* Register OpenThread receive callback */
    otIp6SetReceiveCallback(ot_, otReceiveCb, this);
    otIp6SetReceiveFilterEnabled(ot_, true);

    /* Register OpenThread state changed callback */
    otSetStateChangedCallback(ot_, otStateChangedCb, this);
}

OpenThreadNetif::~OpenThreadNetif() {
    {
        ot::ThreadLock lk;
        /* Unregister OpenThread state changed and receive callbacks */
        otRemoveStateChangeCallback(ot_, otStateChangedCb, this);
        otIp6SetReceiveCallback(ot_, nullptr, nullptr);
    }
    netifapi_netif_remove(interface());
}

/* LwIP netif init callback */
err_t OpenThreadNetif::initCb(netif* netif) {
    netif->name[0] = 't';
    netif->name[1] = 'h';

    netif->mtu = 1280;
    netif->output_ip6 = outputIp6Cb;
    netif->rs_count = 0;
    netif->flags |= NETIF_FLAG_NO_ND6;
    netif->mld_mac_filter = mldMacFilterCb;

    /* FIXME */
    netif_set_default(netif);

    return ERR_OK;
}

/* LwIP netif output_ip6 callback */
err_t OpenThreadNetif::outputIp6Cb(netif* netif, pbuf* p, const ip6_addr_t* addr) {
    auto self = static_cast<OpenThreadNetif*>(netif->state);

    ot::ThreadLock lk;

    ip_addr_t src = {};
    ip_addr_t dst = {};
    struct ip6_hdr* ip6hdr = (struct ip6_hdr *)p->payload;
    ip_addr_copy_from_ip6_packed(dst, ip6hdr->dest);
    ip_addr_copy_from_ip6_packed(src, ip6hdr->src);

    // char tmp[IP6ADDR_STRLEN_MAX] = {0};
    // char tmp1[IP6ADDR_STRLEN_MAX] = {0};

    // ipaddr_ntoa_r(&src, tmp, sizeof(tmp));
    // ipaddr_ntoa_r(&dst, tmp1, sizeof(tmp1));

    // LOG(TRACE, "OpenThreadNetif(%x) output() %lu bytes (%s -> %s)", self, p->tot_len, tmp, tmp1);

    otMessageSettings settings = {};
    settings.mLinkSecurityEnabled = 1;
    settings.mPriority = OT_MESSAGE_PRIORITY_NORMAL;

    auto msg = otIp6NewMessage(self->ot_, &settings);
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

err_t OpenThreadNetif::mldMacFilterCb(netif* netif, const ip6_addr_t *group,
        netif_mac_filter_action action) {
    auto self = static_cast<OpenThreadNetif*>(netif->state);

    ot::ThreadLock lk;

    otIp6Address addr = {};
    ip6AddrToOtIp6Address(*group, &addr);

    int ret = OT_ERROR_NONE;

    // IMPORTANT: it's no longer possible to manage multicast
    // subscriptions while the OpenThread netif is not fully up,
    // as this will trigger an assertion failure in Netif::SubscribeAllNodesMulticast(void),
    // hence otIp6IsEnabled() check.

    // A very hacky solution: keep the list of subscriptions on loopback
    if (action == NETIF_ADD_MAC_FILTER) {
        mld6_joingroup_netif(netif_get_by_index(1), group);
        if (otIp6IsEnabled(self->ot_)) {
            ret = otIp6SubscribeMulticastAddress(self->ot_, &addr);
        }
    } else if (action == NETIF_DEL_MAC_FILTER) {
        mld6_leavegroup_netif(netif_get_by_index(1), group);
        if (otIp6IsEnabled(self->ot_)) {
            ret = otIp6UnsubscribeMulticastAddress(self->ot_, &addr);
        }
    }

    return ret == OT_ERROR_NONE ? ERR_OK : ERR_VAL;
}

/* OpenThread receive callback */
void OpenThreadNetif::otReceiveCb(otMessage* msg, void* ctx) {
    auto self = static_cast<OpenThreadNetif*>(ctx);

    ot::ThreadLock lk;

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

void OpenThreadNetif::debugLogOtStateChange(uint32_t flags) {
    if (flags & OT_CHANGED_IP6_ADDRESS_ADDED) {
        LOG_DEBUG(TRACE, "OT_CHANGED_IP6_ADDRESS_ADDED");
    }
    if (flags & OT_CHANGED_IP6_ADDRESS_REMOVED) {
        LOG_DEBUG(TRACE, "OT_CHANGED_IP6_ADDRESS_REMOVED");
    }
    if (flags & OT_CHANGED_THREAD_ROLE) {
        LOG_DEBUG(TRACE, "OT_CHANGED_THREAD_ROLE");
    }
    if (flags & OT_CHANGED_THREAD_LL_ADDR) {
        LOG_DEBUG(TRACE, "OT_CHANGED_THREAD_LL_ADDR");
    }
    if (flags & OT_CHANGED_THREAD_ML_ADDR) {
        LOG_DEBUG(TRACE, "OT_CHANGED_THREAD_ML_ADDR");
    }
    if (flags & OT_CHANGED_THREAD_RLOC_ADDED) {
        LOG_DEBUG(TRACE, "OT_CHANGED_THREAD_RLOC_ADDED");
    }
    if (flags & OT_CHANGED_THREAD_RLOC_REMOVED) {
        LOG_DEBUG(TRACE, "OT_CHANGED_THREAD_RLOC_REMOVED");
    }
    if (flags & OT_CHANGED_THREAD_PARTITION_ID) {
        LOG_DEBUG(TRACE, "OT_CHANGED_THREAD_PARTITION_ID");
    }
    if (flags & OT_CHANGED_THREAD_KEY_SEQUENCE_COUNTER) {
        LOG_DEBUG(TRACE, "OT_CHANGED_THREAD_KEY_SEQUENCE_COUNTER");
    }
    if (flags & OT_CHANGED_THREAD_CHILD_ADDED) {
        LOG_DEBUG(TRACE, "OT_CHANGED_THREAD_CHILD_ADDED");
    }
    if (flags & OT_CHANGED_THREAD_CHILD_REMOVED) {
        LOG_DEBUG(TRACE, "OT_CHANGED_THREAD_CHILD_REMOVED");
    }
    if (flags & OT_CHANGED_IP6_MULTICAST_SUBSRCRIBED) {
        LOG_DEBUG(TRACE, "OT_CHANGED_IP6_MULTICAST_SUBSRCRIBED");
    }
    if (flags & OT_CHANGED_IP6_MULTICAST_UNSUBSRCRIBED) {
        LOG_DEBUG(TRACE, "OT_CHANGED_IP6_MULTICAST_UNSUBSRCRIBED");
    }
    if (flags & OT_CHANGED_COMMISSIONER_STATE) {
        LOG_DEBUG(TRACE, "OT_CHANGED_COMMISSIONER_STATE");
    }
    if (flags & OT_CHANGED_JOINER_STATE) {
        LOG_DEBUG(TRACE, "OT_CHANGED_JOINER_STATE");
    }
    if (flags & OT_CHANGED_THREAD_CHANNEL) {
        LOG_DEBUG(TRACE, "OT_CHANGED_THREAD_CHANNEL");
    }
    if (flags & OT_CHANGED_THREAD_PANID) {
        LOG_DEBUG(TRACE, "OT_CHANGED_THREAD_PANID");
    }
    if (flags & OT_CHANGED_THREAD_NETWORK_NAME) {
        LOG_DEBUG(TRACE, "OT_CHANGED_THREAD_NETWORK_NAME");
    }
    if (flags & OT_CHANGED_THREAD_EXT_PANID) {
        LOG_DEBUG(TRACE, "OT_CHANGED_THREAD_EXT_PANID");
    }
    if (flags & OT_CHANGED_MASTER_KEY) {
        LOG_DEBUG(TRACE, "OT_CHANGED_MASTER_KEY");
    }
    if (flags & OT_CHANGED_PSKC) {
        LOG_DEBUG(TRACE, "OT_CHANGED_PSKC");
    }
    if (flags & OT_CHANGED_SECURITY_POLICY) {
        LOG_DEBUG(TRACE, "OT_CHANGED_SECURITY_POLICY");
    }
}

void OpenThreadNetif::stateChanged(uint32_t flags) {
    debugLogOtStateChange(flags);

    LwipTcpIpCoreLock lk;

    if (!netif_is_up(interface())) {
        return;
    }

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
                refreshIpAddresses();

                // FIXME:
                flags |= OT_CHANGED_IP6_MULTICAST_SUBSRCRIBED;
            }
        }
    }

    if (flags & OT_CHANGED_THREAD_NETDATA) {
        LOG(TRACE, "OT_CHANGED_THREAD_NETDATA");
        refreshIpAddresses();
    }

    /* IPv6-addresses */
    if (flags & (OT_CHANGED_IP6_ADDRESS_ADDED |
                 OT_CHANGED_IP6_ADDRESS_REMOVED |
                 OT_CHANGED_THREAD_LL_ADDR |
                 OT_CHANGED_THREAD_ML_ADDR)) {
        syncIpState();
    }

    if (flags & (OT_CHANGED_IP6_MULTICAST_SUBSRCRIBED | OT_CHANGED_IP6_MULTICAST_UNSUBSRCRIBED)) {
        /* Sychronize multicast groups */
        /* FIXME: naive */
        /* Clear all groups */
        interface()->mld_mac_filter = nullptr;
        mld6_stop(interface());
        for (auto addr = otIp6GetMulticastAddresses(ot_); addr; addr = addr->mNext) {
            ip6_addr_t ip6addr = {};
            otNetifMulticastAddressToIp6Addr(addr, ip6addr);
            mld6_joingroup_netif(interface(), &ip6addr);
#ifdef DEBUG_BUILD
            char tmp[IP6ADDR_STRLEN_MAX] = {0};
            ip6addr_ntoa_r(&ip6addr, tmp, sizeof(tmp));
            LOG_DEBUG(TRACE, "Subscribed to %s", tmp);
#endif // DEBUG_BUILD
        }
        // Go through the list of subscriptions on LwIP side and add them if needed
        for (struct mld_group* g = netif_mld6_data(netif_get_by_index(1)); g != nullptr; g = g->next) {
            otIp6Address addr = {};
            ip6AddrToOtIp6Address(g->group_address, &addr);
            otIp6SubscribeMulticastAddress(ot_, &addr);
            if (!mld6_lookfor_group(interface(), &g->group_address)) {
                mld6_joingroup_netif(interface(), &g->group_address);
            }
        }
        interface()->mld_mac_filter = mldMacFilterCb;
    }
}

void OpenThreadNetif::refreshIpAddresses() {
    /* Remove addresses that don't match any prefix in the network data */
    for (unsigned i = 0; i < sizeof(addresses_) / sizeof(addresses_[0]); i++) {
        if (!addresses_[i].mValid) {
            continue;
        }
        if (!otNetifAddressInNetDataPrefixList(ot_, addresses_[i], OT_NETIF_ADDRESS_TYPE_SLAAC)) {
            /* Remove */
            otIp6RemoveUnicastAddress(ot_, &addresses_[i].mAddress);
            memset(&addresses_[i], 0, sizeof(addresses_[i]));
        }
    }

    const uint16_t ourRloc16 = otThreadGetRloc16(ot_);

    /* Active Border Router election */
    otBorderRouterConfig active = {};

    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otBorderRouterConfig config = {};
    while (otNetDataGetNextOnMeshPrefix(ot_, &iterator, &config) == OT_ERROR_NONE) {
        /* For now we are only handling SLAAC prefixes */
        if (!config.mSlaac) {
            continue;
        }

        /* Do not process prefixes with P_default = false here, we are trying to choose
         * the best BR.
         */
        if (!config.mDefaultRoute) {
            continue;
        }

        if (config.mPrefix.mLength > 64) {
            LOG(WARN, "Longer than /64 prefixes are currently not supported");
            continue;
        }

        /* OpenThread does not export functions that deal with scopes :(
         * Use LwIP here
         */
        ip6_addr_t addr = {};
        otIp6AddressToIp6Addr(&config.mPrefix.mPrefix, addr);
        ip6_addr_t activeAddr = {};
        otIp6AddressToIp6Addr(&active.mPrefix.mPrefix, activeAddr);

        LOG(TRACE, "Candidate preferred prefix: %s/%u, preference = %s, RLOC16 = %04x, preferred = %d, stable = %d",
            IP6ADDR_NTOA(&addr), config.mPrefix.mLength,
            routerPreferenceToString(config.mPreference),
            config.mRloc16,
            config.mPreferred,
            config.mStable);

        /* Rule -1. We are a border router, do not choose any other */
        if (config.mRloc16 == ourRloc16) {
            LOG(TRACE, "This is our own prefix");
            active = config;
            continue;
        } else if (active.mRloc16 == ourRloc16) {
            continue;
        }

        if (otIp6IsAddressUnspecified(&active.mPrefix.mPrefix) && active.mPrefix.mLength == 0) {
            /* Rule 0: any prefix is better than none */
            active = config;
        } else if (config.mStable && !active.mStable) {
            /* Rule 1: Stable prefixes are preferred (P_stable = true) */
            active = config;
        } else if (config.mPreferred && !active.mPreferred) {
            /* Rule 2: Prefer preferred prefixes */
            active = config;
        } else if (config.mPreference > active.mPreference) {
            /* Rule 3: Prefix with maximum P_preference is preferred */
            active = config;
        } else if (ip6_addr_isglobal(&addr) > ip6_addr_isglobal(&activeAddr)) {
            /* Rule 4: Global scope is preferred over site-local (GUA over ULA) */
            active = config;
        } else if (config.mPrefix.mLength < active.mPrefix.mLength) {
            /* Rule 5: Larger prefix is preferred (e.g. between /64 and /120, /64 is preferred) */
            active = config;
        } else {
            if (abr_.mRloc16 && config.mRloc16 == abr_.mRloc16 && otIp6PrefixEquals(config.mPrefix, abr_.mPrefix)) {
                /* Rule 6: If the ABR is among the candidates, it should be preferred */
                /* NOTE: anything other than prefix and RLOC16 is ignored */
                active = config;
            } else if (active.mRloc16 != abr_.mRloc16 && config.mRloc16 < active.mRloc16) {
                /* Rule 7: Lowest RLOC16 is preferred */
                active = config;
            }
        }
    }

    if (!otIp6IsAddressUnspecified(&active.mPrefix.mPrefix)) {
        if (memcmp(&abr_, &active, sizeof(active))) {
            ip6_addr_t addr = {};
            otIp6AddressToIp6Addr(&active.mPrefix.mPrefix, addr);
            LOG(INFO, "Switched over to a new preferred prefix: %s/%u, preference = %s, RLOC16 = %04x, preferred = %d, stable = %d",
                IP6ADDR_NTOA(&addr), active.mPrefix.mLength,
                routerPreferenceToString(active.mPreference),
                active.mRloc16,
                active.mPreferred,
                active.mStable);
            abr_ = active;

            if (abr_.mRloc16 != ourRloc16) {
                /* Pref::1 */
                addr.addr[3] = PP_HTONL(1);
                setDns(&addr);
            } else {
                setDns(nullptr);
            }

            if (abr_.mPreference == OT_ROUTE_PREFERENCE_LOW) {
                updateIp6CloudKeepalive(HAL_PLATFORM_BORON_CLOUD_KEEPALIVE_INTERVAL);
            } else {
                updateIp6CloudKeepalive(HAL_PLATFORM_DEFAULT_CLOUD_KEEPALIVE_INTERVAL);
            }
        }
    } else {
        memset(&abr_, 0, sizeof(abr_));
    }

    bool sync = false;

    // Go over the current list of addresses and temporarily set all of them to DEPRECATED
    for (auto& addr : addresses_) {
        if (addr.mValid && addr.mPreferred) {
            addr.mPreferred = false;
            // IMPORTANT: calling otIp6AddUnicastAddress for an address already present in the list
            // will not cause any kind of an event even if for example its state is being changed!
            otIp6AddUnicastAddress(ot_, &addr);

            sync = true;
        }
    }

    // Manually synchronize state with LwIP to notify subscribers for a potential change in IP configuration
    // This should NOT cause any active connections to be dropped (new connections might very temporarily be affected until we
    // set at least one of the addresses as PREFERRED)
    if (sync) {
        syncIpState();
    }

    sync = false;
    iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    while (otNetDataGetNextOnMeshPrefix(ot_, &iterator, &config) == OT_ERROR_NONE) {
        /* For now we are only handling SLAAC prefixes */
        if (!config.mSlaac) {
            continue;
        }

        bool matches = false;

        for (unsigned i = 0; i < sizeof(addresses_) / sizeof(addresses_[0]); i++) {
            if (!addresses_[i].mValid) {
                continue;
            }

            if (otNetifAddressMatchesPrefix(addresses_[i], config.mPrefix)) {
                matches = true;

                if (config.mRloc16 == ourRloc16) {
                    // Make sure that if this is our prefix, we have Pref::1/Preflen address
                    // instead of a random one, because that's where the devices within the mesh
                    // expect DNS server to be at the moment.
                    auto addr = config.mPrefix.mPrefix;
                    addr.mFields.m8[OT_IP6_ADDRESS_SIZE - 1] = 0x01;
                    if (memcmp(&addr, &addresses_[i].mAddress, sizeof(addr))) {
                        otIp6RemoveUnicastAddress(ot_, &addresses_[i].mAddress);
                        addresses_[i] = {};
                        matches = false;
                    }
                }

                if (matches) {
                    bool preferred = otNetifAddressMatchesPrefix(addresses_[i], active.mPrefix);

                    if (addresses_[i].mPreferred != preferred) {
                        addresses_[i].mPreferred = true;
                        // IMPORTANT: calling otIp6AddUnicastAddress for an address already present in the list
                        // will not cause any kind of an event even if for example its state is being changed!
                        otIp6AddUnicastAddress(ot_, &addresses_[i]);
                        sync = true;
                    }
                }
            }
        }

        if (matches) {
            continue;
        }

        /* Add new address */
        for (unsigned i = 0; i < sizeof(addresses_) / sizeof(addresses_[0]); i++) {
            if (addresses_[i].mValid) {
                continue;
            }

            addresses_[i].mAddress = config.mPrefix.mPrefix;
            addresses_[i].mPrefixLength = config.mPrefix.mLength;
            addresses_[i].mValid = true;
            addresses_[i].mPreferred = !memcmp(&config, &active, sizeof(config));
            if (config.mRloc16 != ourRloc16) {
                otIp6CreateRandomIid(ot_, &addresses_[i], nullptr);
            } else {
                /* Pref::1/PrefLen */
                addresses_[i].mAddress.mFields.m8[OT_IP6_ADDRESS_SIZE - 1] = 0x01;
            }
            otIp6AddUnicastAddress(ot_, &addresses_[i]);
            // The call above already generates OT_CHANGED_IP6_ADDRESS_ADDED, so no state sync is required after this point
            sync = false;
            break;
        }
    }

    // Manually synchronize state with LwIP to notify subscribers for a potential change in IP configuration
    if (sync) {
        syncIpState();
    }
}

void OpenThreadNetif::syncIpState() {
    LOG(TRACE, "Synchronizing IP state with LwIP");

    /* Synchronize IP-addresses */
    uint8_t handledLwip[LWIP_IPV6_NUM_ADDRESSES] = {};
    uint8_t handledOt[LWIP_IPV6_NUM_ADDRESSES] = {};

    /* Check existing addresses and adjust state if necessary */
    int otIdx = 0;
    for (auto addr = otIp6GetUnicastAddresses(ot_); addr; addr = addr->mNext) {
        // Skip RLOC addresses
        if (otNetifAddressIsRlocOrAloc(addr)) {
            continue;
        }
        ip6_addr_t ip6addr = {};
        otNetifAddressToIp6Addr(addr, ip6addr);
        if (addr->mScopeOverrideValid) {
            /* FIXME: we should use custom scopes */
            ip6_addr_set_zone(&ip6addr, netif_get_index(interface()));
        } else {
            ip6_addr_assign_zone(&ip6addr, IP6_UNICAST, interface());
        }
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
                char tmp[IP6ADDR_STRLEN_MAX] = {};
                ip6addr_ntoa_r(netif_ip6_addr(interface(), i), tmp, sizeof(tmp));
                netif_ip6_addr_set_state(interface(), i, IP6_ADDR_INVALID);
                netif_ip6_addr_set_parts(interface(), i, 0, 0, 0, 0);
                LOG(TRACE, "Removed %s", tmp);
            }
        }
    }

    /* Add new addresses */
    otIdx = 0;
    for (auto addr = otIp6GetUnicastAddresses(ot_); addr; addr = addr->mNext) {
        if (otNetifAddressIsRlocOrAloc(addr)) {
            continue;
        }
        if (!handledOt[otIdx]) {
            /* New address */
            ip6_addr_t ip6addr = {};
            otNetifAddressToIp6Addr(addr, ip6addr);
            ip6_addr_assign_zone(&ip6addr, IP6_UNICAST, interface());
            const auto state = otNetifAddressStateToIp6AddrState(addr);

            int8_t idx = -1;
            netif_add_ip6_address(interface(), &ip6addr, &idx);
            if (idx >= 0) {
                /* IMPORTANT: scope needs to be adjusted first */
                if (addr->mScopeOverrideValid) {
                    /* FIXME: we should use custom scopes */
                    ip6_addr_set_zone((ip6_addr_t*)netif_ip6_addr(interface(), idx), netif_get_index(interface()));
                } else {
                    ip6_addr_assign_zone((ip6_addr_t*)netif_ip6_addr(interface(), idx), IP6_UNICAST, interface());
                }
                netif_ip6_addr_set_state(interface(), idx, state);
            }
            char tmp[IP6ADDR_STRLEN_MAX] = {0};
            ip6addr_ntoa_r(&ip6addr, tmp, sizeof(tmp));
            LOG(TRACE, "Added %s %d", tmp, otNetifAddressIsRlocOrAloc(addr));
        }
        otIdx++;
    }
}

void OpenThreadNetif::input(otMessage* msg) {
    if (!(netif_is_up(interface()) && netif_is_link_up(interface()))) {
        return;
    }
    uint16_t len = otMessageGetLength(msg);
    //LOG(TRACE, "OpenThreadNetif(%x): input() length %u", this, len);
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

otInstance* OpenThreadNetif::getOtInstance() {
    return ot_;
}

int OpenThreadNetif::up() {
    ot::ThreadLock lk;

    down();

    /* Clean up a set of SLAAC addresses */
    memset(addresses_, 0, sizeof(addresses_));

    LOG(INFO, "Bringing OpenThreadNetif up");
    LOG(INFO, "Network name: %s", otThreadGetNetworkName(ot_));
    LOG(INFO, "802.15.4 channel: %d", (int)otLinkGetChannel(ot_));
    LOG(INFO, "802.15.4 PAN ID: 0x%04x", (unsigned)otLinkGetPanId(ot_));
    int r = 0;
    if ((r = otIp6SetEnabled(ot_, true)) != OT_ERROR_NONE) {
        return r;
    }
    if ((r = otThreadSetEnabled(ot_, true)) != OT_ERROR_NONE) {
        return r;
    }

    return r;
}

int OpenThreadNetif::down() {
    int r = 0;
    {
        ot::ThreadLock lk;
        LOG(INFO, "Bringing OpenThreadNetif down");

        if ((r = otThreadSetEnabled(ot_, false)) != OT_ERROR_NONE) {
            return r;
        }
        if ((r = otIp6SetEnabled(ot_, false)) != OT_ERROR_NONE) {
            return r;
        }
    }

    /* Force link-down */
    LwipTcpIpCoreLock lkk;
    netif_set_link_down(interface());

    return r;
}

int OpenThreadNetif::powerUp() {
    return 0;
}

int OpenThreadNetif::powerDown() {
    return down();
}

void OpenThreadNetif::ifEventHandler(const if_event* ev) {
    if (ev->ev_type == IF_EVENT_STATE) {
        if (ev->ev_if_state->state) {
            up();
        } else {
            down();
        }
    }
}

void OpenThreadNetif::netifEventHandler(netif_nsc_reason_t reason, const netif_ext_callback_args_t* args) {
    /* Nothing to do here */
}

void OpenThreadNetif::setDns(const ip6_addr_t* addr) {
    LwipTcpIpCoreLock lk;
    if (addr) {
        ip_addr_t ipaddr = {};
        ip_addr_copy_from_ip6(ipaddr, *addr);
        dns_setserver(THREAD_DNS_SERVER_INDEX, &ipaddr);
        LOG(INFO, "DNS server on mesh network: %s", IPADDR_NTOA(&ipaddr));
    } else {
        dns_setserver(THREAD_DNS_SERVER_INDEX, nullptr);
        LOG(INFO, "No DNS server on mesh network");
    }
}
