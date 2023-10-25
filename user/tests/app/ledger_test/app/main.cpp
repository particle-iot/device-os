#include <application.h>

#include "request_handler.h"
#include "logger.h"
#include "config.h"

PRODUCT_VERSION(1)

SYSTEM_MODE(SEMI_AUTOMATIC)
// SYSTEM_THREAD(ENABLED)

using namespace particle::test;

namespace {

RequestHandler g_reqHandler;

void onCloudStatus(system_event_t /* event */, int status) {
    switch (status) {
    case cloud_status_disconnected: {
        Log.info("Disconnected");
        break;
    }
    case cloud_status_connecting: {
        Log.info("Connecting");
        break;
    }
    case cloud_status_connected: {
        Log.info("Connected");
        break;
    }
    case cloud_status_disconnecting: {
        Log.info("Disconnecting");
        break;
    }
    default:
        break;
    }
}

} // namespace

void ctrl_request_custom_handler(ctrl_request* req) {
    g_reqHandler.handleRequest(req);
}

void setup() {
    waitFor(Serial.isConnected, 3000);
    int r = initLogger();
    SPARK_ASSERT(r == 0);
    auto& conf = Config::get();
    if (conf.removeAllLedgers) {
        conf.removeAllLedgers = false;
        Log.info("Removing all ledgers");
        r = ledger_purge_all(nullptr);
        if (r < 0) {
            Log.error("ledger_purge_all() failed: %d", r);
        }
    }
    if (conf.removeLedger) {
        conf.removeLedger = false;
        Log.info("Removing ledger: %s", conf.removeLedgerName);
        r = ledger_purge(conf.removeLedgerName, nullptr);
        if (r < 0) {
            Log.error("ledger_purge() failed: %d", r);
        }
    }
    System.on(cloud_status, onCloudStatus);
    if ((!conf.restoreConnection && conf.autoConnect) || (conf.restoreConnection && conf.wasConnected)) {
        Particle.connect();
    }
    conf.restoreConnection = false;
}

void loop() {
}
