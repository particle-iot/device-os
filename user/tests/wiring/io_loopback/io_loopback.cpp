#include "application.h"
#include "unit-test/unit-test.h"
#include <stdlib.h>
#include <utility>
#include <vector>

using namespace std;

/*
 * Follow the instructions in the README.md to connect the correct pin pairs
 */

vector<pair<int,int>> pinPairs = {

// Core, Photon, P1, Electron
#if PLATFORM_ID==0 || PLATFORM_ID==6 || PLATFORM_ID==8 || PLATFORM_ID==10
    make_pair(A0, A1),
    make_pair(A2, A3),
    make_pair(A4, A5),
    make_pair(DAC, WKP),
    make_pair(RX, TX),
    make_pair(D0, D1),
    make_pair(D2, D3),
    make_pair(D4, D5),
    make_pair(D6, D7),
#endif // PLATFORM_ID==0 || PLATFORM_ID==6 || PLATFORM_ID==8 || PLATFORM_ID==10

#if PLATFORM_ID==10
    make_pair(B0, B1),
    make_pair(B2, B3),
    make_pair(B4, B5),
    make_pair(C0, C1),
    make_pair(C2, C3),
    make_pair(C4, C5),
#endif // PLATFORM_ID==10

#if PLATFORM_ID==8
// My test P1 device, the Sparkfun RedBoard doesn't expose P1S0-P1S5
#if 0
    make_pair(P1S0, P1S1),
    make_pair(P1S2, P1S3),
    make_pair(P1S4, P1S5),
#endif
#endif // PLATFORM_ID==8

#if PLATFORM_ID==31

    // Broadcom pin numbers
    make_pair(GPIO4, GPIO17),
    make_pair(GPIO27, GPIO22),
    make_pair(GPIO5, GPIO6),
    make_pair(GPIO13, GPIO19),
    make_pair(GPIO19, GPIO26),
    make_pair(GPIO18, GPIO25),
    make_pair(GPIO23, GPIO24),
    make_pair(GPIO12, GPIO16),
    make_pair(GPIO20, GPIO21),

    // Particle pin numbers
    make_pair(D0, D1),
    make_pair(D2, D3),
    make_pair(D4, D5),
    make_pair(D6, D7),
    make_pair(D7, D8),
    make_pair(D9, D12),
    make_pair(D10, D11),
    make_pair(D13, D14),
    make_pair(D15, D16),

    make_pair(A0, A3),
    make_pair(A1, A2),
    make_pair(A4, A5),
    make_pair(A6, A7),

    // Peripheral pins on the Raspberry Pi don't work for GPIO
    //make_pair(SDA, SCL),
    //make_pair(MOSI, MISO),
    //make_pair(SCK, EED),
    //make_pair(TX, RX),
#endif // PLATFORM_ID==31

};

test(IO_DigitalReadWrite) {
    for (auto p : pinPairs) {
        INFO("Pair %d %d ", p.first, p.second);

        // first outputs to second
        pinMode(p.second, INPUT);
        pinMode(p.first, OUTPUT);

        digitalWrite(p.first, HIGH);
        delay(1);
        assertTrue(digitalRead(p.second) == HIGH);

        digitalWrite(p.first, LOW);
        delay(1);
        assertTrue(digitalRead(p.second) == LOW);

        // second outputs to first
        pinMode(p.first, INPUT);
        pinMode(p.second, OUTPUT);

        digitalWrite(p.second, HIGH);
        delay(1);
        assertTrue(digitalRead(p.first) == HIGH);

        digitalWrite(p.second, LOW);
        delay(1);
        assertTrue(digitalRead(p.first) == LOW);

        pinMode(p.second, INPUT);
    }
}

test(IO_PullUpPullDown) {
    for (auto p : pinPairs) {
        INFO("Pair %d %d ", p.first, p.second);

        // first pulls up to second
        pinMode(p.second, INPUT);
        pinMode(p.first, INPUT_PULLUP);
        delay(1);
        assertTrue(digitalRead(p.second) == HIGH);

        // first pulls down to second
        pinMode(p.first, INPUT_PULLDOWN);
        delay(1);
        assertTrue(digitalRead(p.second) == LOW);

        pinMode(p.first, INPUT);
        bool skipTX = PLATFORM_ID == 8 || PLATFORM_ID == 10;
        if (skipTX && p.second == TX) {
            // Electron and P1 have an issue with pullup on the TX pin
        } else {
            // second pulls up to first
            pinMode(p.second, INPUT_PULLUP);
            delay(1);
            assertTrue(digitalRead(p.first) == HIGH);

            // second pulls down to first
            pinMode(p.second, INPUT_PULLDOWN);
            delay(1);
            assertTrue(digitalRead(p.first) == LOW);
        }

        pinMode(p.second, INPUT);
    }
}
