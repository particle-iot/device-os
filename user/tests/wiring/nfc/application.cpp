#include "application.h"

SYSTEM_MODE(AUTOMATIC);

Serial1LogHandler logHandler(115200, LOG_LEVEL_WARN, {{"nfc", LOG_LEVEL_ALL}});

static void nfc_event_callback(nfc_event_type_t type, nfc_event_t* event, void* ctx) {
    switch (type) {
        case NFC_EVENT_FIELD_ON: {
            digitalWrite(D7, 1);
            break;
        }
        case NFC_EVENT_FIELD_OFF: {
            digitalWrite(D7, 0);
            digitalWrite(D0, 0);
            break;
        }
        case NFC_EVENT_READ: {
            digitalWrite(D0, 1);
            break;
        }
        default:
            break;
    }
}

int cloudUri(String command) {
    NFC.setUri(command, NfcUriType::NFC_URI_HTTPS_WWW);
    NFC.update();
    Particle.publish("uri/event",command);
    return 0;
}

void setup (void) {
    pinMode(D0, OUTPUT);
    digitalWrite(D0, 0);
    
    // HAllo
    pinMode(D7, OUTPUT);
    digitalWrite(D7, 0);
    
    // To simplify the API, only support one record
    // NFC.setText("Hello Particle!", "en");
    NFC.setUri("particle.io", NfcUriType::NFC_URI_HTTPS_WWW);
    // NFC.setLaunchApp("io.particle.android.app");
    
    NFC.on(nfc_event_callback);
    Particle.function("nfc_uri", cloudUri);
}

void loop(){

}
