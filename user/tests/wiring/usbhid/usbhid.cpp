#include "application.h"
#include "unit-test/unit-test.h"

void startup();
STARTUP(startup());

void startup()
{
    Keyboard.begin();
    Mouse.begin();
}

int randomString(char *buf, int len) {
    for (int i = 0; i < len; i++) {
        uint8_t d = random(0, 15);
        char c = d + 48;
        if (57 < c)
            c += 7;
        buf[i] = c;
    }

    return len;
}

void consume(Stream& serial)
{
    while (serial.available() > 0) {
        (void)serial.read();
    }
}

test(0_USBHID_MultipleConsecutiveReportsAreSuccessfullyDelivered) {
    Serial.println("Please keep the terminal window in focus");
    delay(5000);
    srand(millis());

    assertEqual(Serial.available(), 0);

    char randStr[65];
    char message[128];
    memset(randStr, 0, sizeof(randStr));
    memset(message, 0, sizeof(message));
    randomString(randStr, sizeof(randStr) - 1);
    Serial.println(randStr);

    Keyboard.println(randStr);

    serialReadLine(&Serial, message, sizeof(message) - 1, 10000); //10 sec timeout
    assertTrue(strncmp(randStr, message, sizeof(randStr) - 1) == 0);
    Serial.println();

    randomString(randStr, sizeof(randStr) - 1);
    Serial.println(randStr);
    memset(message, 0, sizeof(message));
    consume(Serial);

    for (int i = 0; i < sizeof(randStr) - 1; i++) {
        Keyboard.print(randStr[i]);
        Mouse.move(5, 0, 0);
    }
    Keyboard.println();

    serialReadLine(&Serial, message, sizeof(message) - 1, 10000); //10 sec timeout
    assertTrue(strncmp(randStr, message, sizeof(randStr) - 1) == 0);
    Serial.println();
}
