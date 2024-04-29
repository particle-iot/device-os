#include "application.h"
#include "unit-test/unit-test.h"

SYSTEM_MODE(MANUAL);

Serial1LogHandler logHandler(115200, LOG_LEVEL_ALL);

const char* ssid = "PCN";
const char* correctPassword = "makeitparticle!";
const char* wrongPassword = "sdghjsgdgh";
const SecurityType secType = SecurityType::WPA2;

WiFiCredentials credentials = {};

enum WiFiState {
    OFF = 0,
    ON = 1,
    CONNECTED = 2,
    MAX = 3
};
    
test(Test_00_Init) {
    credentials.setSsid(ssid, strlen(ssid));
    credentials.setSecurity(secType);
    credentials.setValidate(true);
}

test(Test_01_WiFi_Credentials_Check_And_WiFi_State_Consistency) {
    for (uint8_t test = 0; test < WiFiState::MAX; test++) {
        Serial.printlnf(" > Set WiFi state to %s", (test ? (test == 1 ? "ON" : "CONNECTED") : "OFF"));
        switch (test) {
            case WiFiState::OFF: {
                WiFi.off();
                assertTrue(waitFor(WiFi.isOff, 30000));
                WiFi.clearCredentials();
                assertFalse(WiFi.hasCredentials());
                break;
            }
            case WiFiState::ON: {
                WiFi.on();
                assertTrue(waitFor(WiFi.isOn, 30000));
                WiFi.clearCredentials();
                assertFalse(WiFi.hasCredentials());
                break;
            }
            case WiFiState::CONNECTED: {
                WiFi.connect();
                assertTrue(waitFor(WiFi.ready, 30000));
                break;
            }
        }

        Serial.println("  > Set wrong Wi-Fi credentials");
        credentials.setPassword(wrongPassword, strlen(wrongPassword));
        WiFi.setCredentials(credentials);
        if (test == WiFiState::OFF) {
            assertTrue(WiFi.isOff());
            assertFalse(WiFi.hasCredentials());
        } else if (test == WiFiState::ON) {
            assertTrue(WiFi.isOn());
            assertFalse(WiFi.hasCredentials());
        } else if (test == WiFiState::CONNECTED) {
            // The netif should restore the connection automatically
            assertTrue(waitFor(WiFi.ready, 30000));
            assertTrue(WiFi.hasCredentials());
        }

        Serial.println("  > Set correct Wi-Fi credentials");
        credentials.setPassword(correctPassword, strlen(correctPassword));
        WiFi.setCredentials(credentials);
        if (test == WiFiState::OFF) {
            assertTrue(WiFi.isOff());
        } else if (test == WiFiState::ON) {
            assertTrue(WiFi.isOn());
        } else if (test == WiFiState::CONNECTED) {
            assertTrue(waitFor(WiFi.ready, 30000));
        }
        assertTrue(WiFi.hasCredentials());

#if HAL_PLATFORM_RTL872X
        // FIXME: It runs into SOS with 10 blinks if we frequently turn on/off the Wi-Fi
        delay(5s);
#endif

        // To verify that the credentials are still valid
        if (test == WiFiState::OFF || test == WiFiState::ON) {
            Serial.println("  > Set wrong Wi-Fi credentials");
            credentials.setPassword(wrongPassword, strlen(wrongPassword));
            WiFi.setCredentials(credentials);
            if (test == WiFiState::OFF) {
                assertTrue(WiFi.isOff());
            } else if (test == WiFiState::ON) {
                assertTrue(WiFi.isOn());
            }
            assertTrue(WiFi.hasCredentials());
        }
    }
}
