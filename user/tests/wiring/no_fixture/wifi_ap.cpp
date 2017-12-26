#include "application.h"
#include "unit-test/unit-test.h"

#if Wiring_WiFi_AP

test(wifi_ap_state)
{
    AP.off();
    assertFalse(AP.ready());

    AP.on();
    assertTrue(AP.ready());
}

test(wifi_ap_get_credentials_bssid_when_off)
{
    AP.off();
    WiFiAccessPoint creds;
    AP.getCredentials(creds);
    uint8_t bssid_sum;

    for (int i=0; i<6; i++) {
        bssid_sum |= creds.bssid[i];
    }
    assertTrue(bssid_sum==0);
}

test(wifi_ap_get_credentials_bssid_when_on)
{
    AP.on();
    WiFiAccessPoint creds;
    AP.getCredentials(creds);
    uint8_t bssid_sum;
    for (int i=0; i<6; i++) {
        bssid_sum |= creds.bssid[i];
    }
    assertTrue(bssid_sum!=0);
}

test(wifi_ap_can_set_and_clear_credentials)
{
    AP.off();
    AP.setCredentials("ssid", "password", WPA, WLAN_CIPHER_TKIP);
    assertTrue(AP.hasCredentials());

    WiFiAccessPoint ap;
    memset(&ap, 0, sizeof(ap));
    AP.getCredentials(ap);

    assertEqual(String("ssid"), String(ap.ssid));
    assertEqual(4, ap.ssidLength);

    assertEqual(ap.security, WLAN_SEC_WPA);
    assertEqual(ap.cipher, WLAN_CIPHER_TKIP);

    AP.clearCredentials();
    assertFalse(AP.hasCredentials());

    // even without credentials, retrieving credentials provides some defaults
    memset(&ap, 0, sizeof(ap));
    AP.getCredentials(ap);

}
#endif
