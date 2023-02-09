#include "Particle.h"
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);
Serial1LogHandler log1Handler(115200, LOG_LEVEL_ALL);

retained uint8_t resetRetry = 0;

void setup() {
#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
    // To force Ethernet only, clear Wi-Fi credentials
    Log.info("Clear Wi-Fi credentionals...");
    WiFi.clearCredentials();
#endif // HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY

    // Disable Listening Mode if not required
    if (System.featureEnabled(FEATURE_DISABLE_LISTENING_MODE)) {
        Log.info("FEATURE_DISABLE_LISTENING_MODE enabled");
    } else {
        Log.info("Disabling Listening Mode...");
        System.enableFeature(FEATURE_DISABLE_LISTENING_MODE);
    }

    Log.info("Checking if Ethernet is on...");
    if (Ethernet.isOn()) {
        Log.info("Ethernet is on");
        uint8_t macAddrBuf[8] = {};
        uint8_t* macAddr = Ethernet.macAddress(macAddrBuf);
        if (macAddr != nullptr) {
            Log.info("Ethernet MAC: %02x %02x %02x %02x %02x %02x",
                    macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
        }
        Ethernet.connect();
        waitFor(Ethernet.ready, 30000);
        Log.info("Ethernet.ready: %d", Ethernet.ready());
        resetRetry = 0;
    } else if (++resetRetry <= 3) {
        Log.info("Ethernet is off or not detected, attmpting to remap pins: %d/3", resetRetry);

        if_wiznet_pin_remap remap = {};
        remap.base.type = IF_WIZNET_DRIVER_SPECIFIC_PIN_REMAP;

        remap.cs_pin = PIN_INVALID; // default
        remap.reset_pin = PIN_INVALID; // default
        remap.int_pin = PIN_INVALID; // default

        // remap.cs_pin = D5;
        // remap.reset_pin = D3;
        // remap.int_pin = D4;

        // remap.cs_pin = D2;
        // remap.reset_pin = D0;
        // remap.int_pin = D1;

        // remap.cs_pin = D10;
        // remap.reset_pin = D6;
        // remap.int_pin = D7;

        // remap.cs_pin = A0;
        // remap.reset_pin = A2;
        // remap.int_pin = A1;

        // remap.cs_pin = 50; // bad pin config
        // remap.reset_pin = 100;
        // remap.int_pin = 150;

        auto ret = if_request(nullptr, IF_REQ_DRIVER_SPECIFIC, &remap, sizeof(remap), nullptr);
        if (ret != SYSTEM_ERROR_NONE) {
            Log.error("Ethernet GPIO config error: %d", ret);
        } else {
            if (System.featureEnabled(FEATURE_ETHERNET_DETECTION)) {
                Log.info("FEATURE_ETHERNET_DETECTION enabled");
            } else {
                Log.info("Enabling Ethernet...");
                System.enableFeature(FEATURE_ETHERNET_DETECTION);
            }
            delay(500);
            System.reset();
        }
    }

    Particle.connect();
}

void loop() {
    static system_tick_t lastPublish = millis();
    static int count = 0;
    static bool reconnect = false;

    if (Particle.connected()) {
        reconnect = false;
        if (millis() - lastPublish >= 10000UL) {
            Particle.publish("mytest", String(++count), PRIVATE, WITH_ACK);
            lastPublish = millis();
        }
    }

    // Detect a network dropout and reconnect quickly
    if (!reconnect && !Ethernet.ready()) {
        Log.info("Particle disconnect...");
        Particle.disconnect();
        waitFor(Particle.disconnected, 5000);
        Log.info("Network disconnect...");
        Network.disconnect();
        Particle.connect();
        reconnect = true;
    }
}
