#include "application.h"

SYSTEM_MODE(SEMI_AUTOMATIC);

STARTUP(USBSerial1.begin());

void setup() {
    Cellular.on();
}

void loop() {
    while (true) {
        if (USBSerial1.available() > 0) {
            Cellular.off();
            delay(1000);
            Cellular.on();
            while (USBSerial1.available() > 0) {
                USBSerial1.read();
            }
            USBSerial1.printlnf("OK");
        }

        while (cellular_rx_readable()) {
            Serial.write(cellular_getc());
        }

        while (Serial.available() > 0) {
            cellular_putc(Serial.read());
        }
    }
}
