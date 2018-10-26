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

#ifndef SPARK_WIRING_MESH_H
#define    SPARK_WIRING_MESH_H

#include "spark_wiring_platform.h"
#include "spark_wiring_network.h"
#include "spark_wiring_udp.h"
#include "system_network.h"

#if Wiring_Mesh

#include "spark_wiring_signal.h"
#include "system_task.h"
#include "events.h"
#include "system_error.h"
#include "check.h"
#include "ifapi.h"
#include <memory>
#include "scope_guard.h"

#include "spark_wiring_thread.h"

namespace spark {

class MeshSignal : public particle::Signal {
public:
    // In order to be compatible with CellularSignal
    int rssi = 2;
    int qual = 0;

    MeshSignal() {}
    virtual ~MeshSignal() {};

    operator int8_t() const {
        return 2;
    }

    virtual hal_net_access_tech_t getAccessTechnology() const {
        return NET_ACCESS_TECHNOLOGY_IEEE802154;
    }

    virtual float getStrength() const {
        return 0.0f;
    }

    virtual float getStrengthValue() const {
        return 0.0f;
    }

    virtual float getQuality() const {
        return 0.0f;
    }

    virtual float getQualityValue() const {
        return 0.0f;
    }
};


int mesh_loop();

class MeshPublish {
private:
    class Subscriptions {
        FilteringEventHandler event_handlers[5];

    protected:
        /**
         * Determines if the given handler exists.
         */
        bool event_handler_exists(const char *event_name, EventHandler handler,
                void *handler_data, SubscriptionScope::Enum scope, const char* id);

        /**
         * Adds the given handler.
         */
        int add_event_handler(const char *event_name, EventHandler handler,
                void *handler_data, SubscriptionScope::Enum scope, const char* id);

    public:
        int add(const char* name, EventHandler handler);

        void send(const char* event_name, const char* data);
    };


    static const uint16_t PORT = 36969;
    static constexpr const char* MULTICAST_ADDR = "ff03::1:1001";
    static const uint16_t MAX_PACKET_LEN = 1232;

    std::unique_ptr<UDP> udp;
    Subscriptions subscriptions;

    static int fetchMulticastAddress(IPAddress& mcastAddr);

    int initialize_udp();

    int uninitialize_udp();

    std::unique_ptr<Thread> thread_;
    RecursiveMutex mutex_;
    std::unique_ptr<uint8_t[]> buffer_;

public:

    MeshPublish() : udp(nullptr) {
        // System thread gets blocked while connecting to cloud, while it's connecting to it
        // RX packet buffer pool may easily get exhausted, because nobody is reading the data
        // out of the socket. Create a separate thread here with a higher priority than application
        // and system.
    }

    int publish(const char* topic, const char* data = nullptr);

    int subscribe(const char* prefix, EventHandler handler);

    /**
     * Pull data from the socket and handle as required.
     */
    int poll();
};

class MeshClass : public NetworkClass, public MeshPublish {
public:
    MeshClass() :
            NetworkClass(NETWORK_INTERFACE_MESH) {
    }

    void on() {
        network_on(*this, 0, 0, NULL);
    }

    void off() {
        network_off(*this, 0, 0, NULL);
    }

    void connect(unsigned flags=0) {
        network_connect(*this, flags, 0, NULL);
    }

    bool connecting(void) {
        return network_connecting(*this, 0, NULL);
    }

    void disconnect() {
        network_disconnect(*this, NETWORK_DISCONNECT_REASON_USER, NULL);
    }

    void listen(bool begin=true) {
        network_listen(*this, begin ? 0 : 1, NULL);
    }

    void setListenTimeout(uint16_t timeout) {
        network_set_listen_timeout(*this, timeout, NULL);
    }

    uint16_t getListenTimeout(void) {
        return network_get_listen_timeout(*this, 0, NULL);
    }

    bool listening(void) {
        return network_listening(*this, 0, NULL);
    }

    bool ready() {
        return network_ready(*this, 0,  NULL);
    }

    // There are multiple IPv6 addresses, here we are only reporting ML-EID (Mesh-Local EID)
    IPAddress localIP();
};


extern MeshClass Mesh;

} /* namespace spark */

#endif /* Wiring_Mesh */
#endif /* SPARK_WIRING_MESH_H */
