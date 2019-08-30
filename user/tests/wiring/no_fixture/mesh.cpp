/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#include "application.h"
#include "unit-test/unit-test.h"
#include "random.h"

#if Wiring_Mesh

test(MESH_01_PublishWithoutSubscribeStillReadsDataOutOfPubSubSocket) {
    // https://github.com/particle-iot/device-os/issues/1828

    static const constexpr unsigned MAX_ITERATIONS = 1000;

    if (!network_has_credentials(NETWORK_INTERFACE_MESH, 0, nullptr)) {
        // Simply skip this test if there are no mesh credentials
        skip();
        return;
    }

    // Make sure the device is a known (deinitialized) state
    Mesh.uninitializeUdp();

    // This creates a pubsub socket
    Mesh.publish("test", "123");

    // Bombard ourselves
    std::unique_ptr<UDP> udp(new UDP());
    assertTrue((bool)udp);

    assertTrue(udp->setBuffer(MeshPublish::MAX_PACKET_LEN));
    // Get OpenThread interface index (interface is named "th1" on all Mesh devices)
    uint8_t idx = 0;
    assertEqual(if_name_to_index("th1", &idx), (int)SYSTEM_ERROR_NONE);
    // Create UDP socket and bind to OpenThread interface
    assertTrue((bool)udp->begin(0, idx));

    // Enable multicast loop option
    uint8_t loop = 1;
    assertTrue(sock_setsockopt(udp->socket(), IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) == 0);

    IPAddress addr;
    // Localhost
    addr.raw().v = 6;
    assertEqual(inet_inet_pton(AF_INET6, MeshPublish::MULTICAST_ADDR, addr.raw().ipv6), 1);

    // Send a number of random data packets
    Random rng;
    for (unsigned i = 0; i < MAX_ITERATIONS; i++) {
        rng.gen((char*)udp->buffer(), MeshPublish::MAX_PACKET_LEN);
        udp->sendPacket(udp->buffer(), MeshPublish::MAX_PACKET_LEN, addr, MeshPublish::PORT);
    }

    // Ensure that packet buffers were freed correctly by simply sending a cloud publish
    assertTrue((bool)Particle.publish("test", "123", WITH_ACK));
}

#endif // Wiring_Mesh
