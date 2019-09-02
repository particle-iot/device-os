#include "application.h"

STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY))

namespace {

enum Step {
    RESET_BUTTON = 1,
    SYSTEM_RESET,
    DFU,
    SAFE_MODE,
    OTA,
    LISTENING,
    PARTICLE_DISCONNECT,
    NETWORK_DISCONNECT,
    NETWORK_OFF,
    STOP_MODE,
    DEEP_SLEEP,
    NETWORK_SLEEP,
    DONE
};

const Serial1LogHandler logHandler(115200, LOG_LEVEL_NONE, {
    { "app", LOG_LEVEL_ALL }
});

retained unsigned step = 0;
retained unsigned magic = 0;

bool stepDone = false;

} // unnamed

void setup() {
    if (magic != 0x89abcdef) {
        step = Step::RESET_BUTTON;
        magic = 0x89abcdef;
    } else {
        ++step;
    }
}

void loop() {
    if (stepDone) {
        return;
    }
    switch (step) {
    case Step::RESET_BUTTON: {
        Log.info("%u) Press the reset button", step);
        break;
    }
    case Step::SYSTEM_RESET: {
        Log.info("%u) Resetting via System.reset()", step);
        System.reset();
        break;
    }
    case Step::DFU: {
        Log.info("%u) Resetting via System.dfu()", step);
        Log.info("Reset the device to proceed");
        System.dfu();
        break;
    }
    case Step::SAFE_MODE: {
        Log.info("%u) Resetting via System.enterSafeMode()", step);
        Log.info("Reset the device to proceed");
        System.enterSafeMode();
        break;
    }
    case Step::OTA: {
        Log.info("%u) Flash the same application binary OTA", step);
        break;
    }
    case Step::LISTENING: {
        Log.info("%u) Entering the listening mode", step);
        Log.info("Reset the device to proceed");
        Network.listen();
        break;
    }
    case Step::PARTICLE_DISCONNECT: {
        Log.info("%u) Disconnecting via Particle.disconnect()", step);
        Log.info("Reset the device to proceed");
        Particle.disconnect();
        break;
    }
    case Step::NETWORK_DISCONNECT: {
        Log.info("%u) Disconnecting from the network", step);
        Log.info("Reset the device to proceed");
        Network.disconnect();
        break;
    }
    case Step::NETWORK_OFF: {
        Log.info("%u) Turning the network interface off", step);
        Log.info("Reset the device to proceed");
        Network.off();
        break;
    }
    case Step::STOP_MODE: {
        Log.info("%u) Entering the stop mode", step);
        Log.info("Reset the device to proceed");
        System.sleep(D1, RISING, 3);
        break;
    }
    case Step::DEEP_SLEEP: {
        Log.info("%u) Entering the deep sleep mode", step);
#if HAL_PLATFORM_NRF52840
        Log.info("Wake the device up via the D8 pin");
        System.sleep(SLEEP_MODE_DEEP);
#else
        System.sleep(SLEEP_MODE_DEEP, 3);
#endif
        break;
    }
    case Step::NETWORK_SLEEP: {
        // FIXME: This mode seems to be broken on Gen 3 platforms
        Log.info("%u) Entering the network sleep mode", step);
        Log.info("Reset the device to proceed");
        System.sleep(3);
        break;
    }
    case Step::DONE: {
        Log.info("%u) Done", step);
        break;
    }
    default:
        break;
    }
    stepDone = true;
}
