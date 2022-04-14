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

#pragma once

#include "application.h"
#include "scope_guard.h"

namespace particle {
namespace test {

const char eventNameBase[] = "ble-test";

struct BleTestPeer {
    String deviceId;
    String address;
    volatile bool valid = false;
    // For waitUntil/waitFor
    bool isValid() const {
        return valid;
    }
};

struct BleTestPasskey {
    String passkey;
    volatile bool valid = false;
    // For waitUntil/waitFor
    bool isValid() const {
        return valid;
    }
};

namespace detail {

BleTestPeer gBleTestPeer;
BleTestPasskey gBleTestPasskey;

} // detail

BleTestPeer getBleTestPeer() {
    return detail::gBleTestPeer;
}

bool getBleTestPasskey(String& passkey) {
    if (detail::gBleTestPasskey.valid) {
        passkey = detail::gBleTestPasskey.passkey;
        detail::gBleTestPasskey.valid = false;
        return true;
    }
    return false;
}

bool commonEventHandler(const char* eventName, const char* data) {
    using namespace detail;
    Log.info("%s %s %d", eventName, data, strlen(data));
    JSONObjectIterator it(JSONValue::parse((char*)data, strlen(data)));
    BleTestPeer peer;
    BleTestPasskey passkey;
    while (it.next()) {
        if (it.name() == "deviceId" && it.value().isString()) {
            if (it.value().toString() == System.deviceID()) {
                // Skip
                return false;
            }
            peer.deviceId = String(it.value().toString());
        } else if (it.name() == "address" && it.value().isString()) {
            peer.address = String(it.value().toString());
        } else if (it.name() == "passkey" && it.value().isString()) {
            passkey.passkey = String(it.value().toString());
        }
    }
    if (peer.address != "" && peer.deviceId != "") {
        gBleTestPeer = peer;
        gBleTestPeer.valid = true;
    }
    if (passkey.passkey != "" && peer.deviceId != "") {
        gBleTestPasskey = passkey;
        gBleTestPasskey.valid = true;
    }
    return false;
}

String generateEventName() {
#ifdef PARTICLE_TEST_RUNNER
    return String(eventNameBase) + "/" + System.deviceID();
#else
    return String(eventNameBase) + "/any";
#endif // PARTICLE_TEST_RUNNER
}

String generatePeerEventName() {
#ifdef PARTICLE_TEST_RUNNER
    return String(eventNameBase) + "/" + detail::gBleTestPeer.deviceId;
#else
    return String(eventNameBase) + "/any";
#endif // PARTICLE_TEST_RUNNER

}

bool publishBlePeerInfo() {
    using namespace detail;
    char buf[particle::protocol::MAX_EVENT_DATA_LENGTH] = {};
    JSONBufferWriter json(buf, sizeof(buf) - 1);
    json.beginObject();
    json.name("deviceId").value(System.deviceID());
    json.name("address").value(BLE.address().toString());
    json.endObject();
    Log.info("publish %s %s %d", generateEventName().c_str(), buf, json.dataSize());
    return Particle.publish(generateEventName(), buf, WITH_ACK);
}

bool publishPassKey(const char* passkey, size_t len) {
    using namespace detail;
    String* passkeyData = new String(passkey, len);
    if (!passkeyData) {
        return false;
    }
    auto pub = [](void* data) -> void {
        if (!data) {
            return;
        }
        String* passkey = (String*)data;
        SCOPE_GUARD({
            if (passkey) {
                delete passkey;
            }
        });

        char buf[particle::protocol::MAX_EVENT_DATA_LENGTH] = {};
        JSONBufferWriter json(buf, sizeof(buf) - 1);
        json.beginObject();
        json.name("deviceId").value(System.deviceID());
        json.name("passkey").value(*passkey);
        json.endObject();
        Log.info("publish %s %s", generatePeerEventName().c_str(), buf);
        bool val = Particle.publish(generatePeerEventName(), buf, WITH_ACK);
        (void)val;
    };
    if (application_thread_current(nullptr)) {
        // Application thread
        pub((void*)passkeyData);
        return true;
    } else {
        return application_thread_invoke(pub, (void*)passkeyData, nullptr) == 0;
    }
}

void centralEventHandler(const char *eventName, const char *data) {
    if (commonEventHandler(eventName, data)) {
        return;
    }
}

void peripheralEventHandler(const char *eventName, const char *data) {
    if (commonEventHandler(eventName, data)) {
        return;
    }
}

void subscribeEvents(hal_ble_role_t role) {
    if (role == BLE_ROLE_CENTRAL) {
        Particle.subscribe(generateEventName(), centralEventHandler);
    } else {
        Particle.subscribe(generateEventName(), peripheralEventHandler);
    }
}

} // test
} // particle