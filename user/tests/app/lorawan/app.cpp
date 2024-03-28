#include <application.h>
#include <check.h>

#include <cloud/cloud_new.pb.h>

#include "cloud/protocol.h"
#include "util/buffer.h"
#include "util/protobuf.h"
#include "lorawan.h"

SYSTEM_MODE(SEMI_AUTOMATIC)

using namespace particle::constrained;

namespace {

Protocol g_proto;
Lorawan g_lorawan;

void helloResponseReceived(int error, int result, util::Buffer data) {
    if (error < 0) {
        Log.error("Hello request failed: %d", error);
        return;
    }
    Log.info("Received Hello response; result: %d", result);

    particle_cloud_HelloResponse resp = {};
    int r = decodeProtobuf(data, &resp, &particle_cloud_HelloResponse_msg);
    if (r < 0) {
        Log.error("Failed to decode response: %d", r);
    }
}

int sendHelloRequest() {
    Log.info("Sending Hello request");

    particle_cloud_HelloRequest req = {};
    req.system_version = 1234;
    req.product_version = 1;

    util::Buffer buf;
    CHECK(encodeProtobuf(buf, &req, &particle_cloud_HelloRequest_msg));

    CHECK(g_proto.sendRequest(1 /* type */, buf, helloResponseReceived));

    return 0;
}

} // namespace

void setup() {
    LorawanConfig lorawanConf;
    lorawanConf.connected([]() {
        g_proto.connect();
        sendHelloRequest();
    });
    lorawanConf.messageReceived([](util::Buffer buf, int port) {
        g_proto.receiveMessage(port, std::move(buf));
    });
    g_lorawan.init(lorawanConf);
    g_lorawan.connect();

    ProtocolConfig protoConf;
    protoConf.connected([]() {
        Log.info("Cloud connected");
    });
    protoConf.disconnected([](int error) {
        if (error) {
            Log.error("Cloud disconnected with error: %d", error);
        } else {
            Log.info("Cloud disconnected");
        }
    });
    protoConf.sendMessage([](auto data, int port, auto ackFn) {
        return g_lorawan.send(data, port, ackFn);
    });

    g_proto.init(protoConf);
}

void loop() {
    int r = g_proto.run();
    if (r < 0) {
        Log.error("Protocol::run() failed: %d", r);
    }
    r = g_lorawan.run();
    if (r < 0) {
        Log.error("Lorawan::run() failed: %d", r);
    }
}
