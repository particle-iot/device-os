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
#define	SPARK_WIRING_MESH_H

#include "spark_wiring_platform.h"
#include "spark_wiring_network.h"
#include "system_network.h"

#if Wiring_Mesh

#include "spark_wiring_signal.h"

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


class MeshClass : public NetworkClass {
public:
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
};


extern MeshClass Mesh;

} /* namespace spark */

#endif /* Wiring_Mesh */
#endif /* SPARK_WIRING_MESH_H */
