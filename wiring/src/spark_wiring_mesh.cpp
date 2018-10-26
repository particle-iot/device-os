/*
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

#include "spark_wiring_platform.h"
#include "spark_wiring_mesh.h"

#if Wiring_Mesh

#include <arpa/inet.h>
#include "delay_hal.h"

namespace spark {

int mesh_loop() {
    return spark::Mesh.poll();
}

bool MeshPublish::Subscriptions::event_handler_exists(const char *event_name, EventHandler handler,
        void *handler_data, SubscriptionScope::Enum scope, const char* id)
{
    const int NUM_HANDLERS = sizeof(event_handlers)
            / sizeof(FilteringEventHandler);
    for (int i = 0; i < NUM_HANDLERS; i++)
    {
        if (event_handlers[i].handler == handler
                && event_handlers[i].handler_data == handler_data
                && event_handlers[i].scope == scope)
        {
            const size_t MAX_FILTER_LEN = sizeof(event_handlers[i].filter);
            const size_t FILTER_LEN = strnlen(event_name, MAX_FILTER_LEN);
            if (!strncmp(event_handlers[i].filter, event_name, FILTER_LEN))
            {
                const size_t MAX_ID_LEN =
                        sizeof(event_handlers[i].device_id) - 1;
                const size_t id_len = id ? strnlen(id, MAX_ID_LEN) : 0;
                if (id_len)
                    return !strncmp(event_handlers[i].device_id, id, id_len);
                else
                    return !event_handlers[i].device_id[0];
            }
        }
    }
    return false;
}

/**
 * Adds the given handler.
 */
int MeshPublish::Subscriptions::add_event_handler(const char *event_name, EventHandler handler,
        void *handler_data, SubscriptionScope::Enum scope, const char* id)
{
    if (event_handler_exists(event_name, handler, handler_data, scope, id))
        return SYSTEM_ERROR_NONE;

    const int NUM_HANDLERS = sizeof(event_handlers) / sizeof(FilteringEventHandler);
    for (int i = 0; i < NUM_HANDLERS; i++)
    {
        if (NULL == event_handlers[i].handler)
        {
            const size_t MAX_FILTER_LEN = sizeof(event_handlers[i].filter);
            const size_t FILTER_LEN = strnlen(event_name, MAX_FILTER_LEN);
            memcpy(event_handlers[i].filter, event_name, FILTER_LEN);
            memset(event_handlers[i].filter + FILTER_LEN, 0, MAX_FILTER_LEN - FILTER_LEN);
            event_handlers[i].handler = handler;
            event_handlers[i].handler_data = handler_data;
            event_handlers[i].device_id[0] = 0;
            const size_t MAX_ID_LEN = sizeof(event_handlers[i].device_id) - 1;
            const size_t id_len = id ? strnlen(id, MAX_ID_LEN) : 0;
            memcpy(event_handlers[i].device_id, id, id_len);
            event_handlers[i].device_id[id_len] = 0;
            event_handlers[i].scope = scope;
            return SYSTEM_ERROR_NONE;
        }
    }
    return SYSTEM_ERROR_NO_MEMORY;
}


int MeshPublish::Subscriptions::add(const char* name, EventHandler handler)
{
    return add_event_handler(name, handler, nullptr, SubscriptionScope::MY_DEVICES, nullptr);
}

void MeshPublish::Subscriptions::send(const char* event_name, const char* data)
{
    const size_t event_name_length = strlen(event_name);
    const int NUM_HANDLERS = sizeof(event_handlers) / sizeof(FilteringEventHandler);
    for (int i = 0; i < NUM_HANDLERS; i++)
    {
        if (NULL == event_handlers[i].handler)
        {
            break;
        }
        const size_t MAX_FILTER_LENGTH = sizeof(event_handlers[i].filter);
        const size_t filter_length = strnlen(event_handlers[i].filter,
                MAX_FILTER_LENGTH);

        if (event_name_length < filter_length)
        {
            // does not match this filter, try the next event handler
            continue;
        }

        const int cmp = memcmp(event_handlers[i].filter, event_name,
                filter_length);
        if (0 == cmp)
        {
            system_invoke_event_handler(sizeof(FilteringEventHandler),
                                            &event_handlers[i], (const char*) event_name,
                                            (const char*) data, nullptr);
        }
        // else continue the for loop to try the next handler
    }
}

int MeshPublish::fetchMulticastAddress(IPAddress& mcastAddr) {
    HAL_IPAddress addr = {};
    addr.v = 6;
    inet_inet_pton(AF_INET6, MULTICAST_ADDR, addr.ipv6);
    mcastAddr = addr;
    return 0;
}

int MeshPublish::initialize_udp() {
    std::lock_guard<RecursiveMutex> lk(mutex_);
    if (udp) {
        return SYSTEM_ERROR_NONE;
    }
    udp.reset(new UDP());
    if (!udp) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    udp->setBuffer(MAX_PACKET_LEN);
    // Get OpenThread interface index (     interface is named "th1" on all Mesh devices)
    uint8_t idx = 0;
    if_name_to_index("th1", &idx);
     // Create UDP socket and bind to OpenThread interface
    CHECK(udp->begin(PORT, idx));
    // subscribe to multicast

    IPAddress mcastAddr;
    CHECK(fetchMulticastAddress(mcastAddr));
    udp->joinMulticast(mcastAddr);
    return SYSTEM_ERROR_NONE;
}

int MeshPublish::uninitialize_udp() {
    std::lock_guard<RecursiveMutex> lk(mutex_);
    if (udp) {
        IPAddress mcastAddr;
        fetchMulticastAddress(mcastAddr);
        udp->leaveMulticast(mcastAddr);
        udp.reset();
    }
    return SYSTEM_ERROR_NONE;
}

int MeshPublish::publish(const char* topic, const char* data) {
    // Topic should be defined
    CHECK_TRUE(topic && (strlen(topic) > 0), SYSTEM_ERROR_INVALID_ARGUMENT);

    // Including null-terminator
    const size_t topicLen = strlen(topic) + 1;
    const size_t dataLen = data ? strlen(data) + 1 : 0;

    // topic + data + version should fit within MAX_PACKET_LEN
    CHECK_TRUE(topicLen + dataLen + sizeof(uint8_t) <= MAX_PACKET_LEN,
            SYSTEM_ERROR_TOO_LARGE);

    std::lock_guard<RecursiveMutex> lk(mutex_);
    CHECK(initialize_udp());
    IPAddress mcastAddr;
    CHECK(fetchMulticastAddress(mcastAddr));

    CHECK(udp->beginPacket(mcastAddr, PORT));
    uint8_t version = 0;
    udp->write(&version, 1);
    udp->write((const uint8_t*)topic, topicLen);
    if (dataLen > 0) {
        udp->write((const uint8_t*)data, dataLen);
    }
    CHECK(udp->endPacket());
    return SYSTEM_ERROR_NONE;
}

int MeshPublish::subscribe(const char* prefix, EventHandler handler) {
    std::lock_guard<RecursiveMutex> lk(mutex_);
    if (!thread_) {
        thread_.reset(new (std::nothrow) Thread("meshpub", [](void* ptr) {
            auto self = (MeshPublish*)ptr;
            while (true) {
                self->poll();
            }
        }, this, OS_THREAD_PRIORITY_DEFAULT + 1));
    }
    CHECK(initialize_udp());
    CHECK(subscriptions.add(prefix, handler));
    return SYSTEM_ERROR_NONE;
}

/**
 * Pull data from the socket and handle as required.
 */
int MeshPublish::poll() {
    int result = 0;
    UDP* u = nullptr;
    {
        std::lock_guard<RecursiveMutex> lk(mutex_);
        u = udp.get();
    }
    if (u) {
        if (!buffer_) {
            buffer_.reset(new (std::nothrow) uint8_t[MAX_PACKET_LEN]);
            if (!buffer_) {
                return SYSTEM_ERROR_NO_MEMORY;
            }
        }
        int len = u->receivePacket(buffer_.get(), MAX_PACKET_LEN, 1000);
        if (len > 0) {
            LOG(TRACE, "parse packet %d", len);
            const char* buffer = (const char*)buffer_.get();

            // There should be a version and it should be "0"
            const char version = *buffer++;
            CHECK_TRUE(version == 0, SYSTEM_ERROR_BAD_DATA);
            len -= sizeof(version);

            // Topic should not be empty
            const size_t topicLen = strnlen(buffer, len);
            CHECK_TRUE(topicLen > 0, SYSTEM_ERROR_BAD_DATA);

            const char* topic = buffer;

            len -= topicLen;
            buffer += topicLen;

            // Topic should be terminated by '\0'
            CHECK_TRUE(len > 0, SYSTEM_ERROR_BAD_DATA);
            CHECK_TRUE(*buffer == 0, SYSTEM_ERROR_BAD_DATA);
            // Skip it
            --len;
            buffer++;

            size_t dataLen = 0;
            const char* data = "";
            if (len > 0) {
                // There is data
                dataLen = strnlen(buffer, len);
                data = buffer;
                // Data can be empty
                len -= dataLen;
                buffer += dataLen;
                // Data should be terminated by '\0'
                CHECK_TRUE(len > 0, SYSTEM_ERROR_BAD_DATA);
                CHECK_TRUE(*buffer == 0, SYSTEM_ERROR_BAD_DATA);
                // Skip it
                --len;
                buffer++;
            }
            CHECK_TRUE(len == 0, SYSTEM_ERROR_BAD_DATA);

            std::lock_guard<RecursiveMutex> lk(mutex_);
            subscriptions.send(topic, data);
        } else {
            result = len;
        }
    } else {
        HAL_Delay_Milliseconds(100);
    }
    return result;
}

IPAddress MeshClass::localIP() {
    HAL_IPAddress addr = {};
    addr.v = 6;

    if_t iface = nullptr;
    if (!if_get_by_index((network_interface_t)*this, &iface)) {
        if_addrs* ifAddrList = nullptr;
        if (!if_get_addrs(iface, &ifAddrList)) {
            SCOPE_GUARD({
                if_free_if_addrs(ifAddrList);
            });
            for (if_addrs* i = ifAddrList; i; i = i->next) {
                if (i->if_addr->addr->sa_family == AF_INET6) {
                    const auto in6addr = (const struct sockaddr_in6*)i->if_addr->addr;

                    // ML-EID will be a preferred, scoped, non-linklocal address
                    if (IN6_IS_ADDR_LINKLOCAL(&in6addr->sin6_addr)) {
                        continue;
                    }

                    if (in6addr->sin6_scope_id == 0) {
                        continue;
                    }

                    if (i->if_addr->ip6_addr_data && i->if_addr->ip6_addr_data->state == IF_IP6_ADDR_STATE_PREFERRED) {
                        memcpy(addr.ipv6, in6addr->sin6_addr.s6_addr, sizeof(addr.ipv6));
                        break;
                    }
                }
            }
        }
    }

    return addr;
}

MeshClass Mesh;
} // namespace spark

#endif
