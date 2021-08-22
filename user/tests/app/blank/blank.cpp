/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "application.h"
#include "dct.h"

RtlLogHandler dbg(LOG_LEVEL_ALL);

SYSTEM_MODE(MANUAL);

void setSetupDone() {
    // LOG(INFO, "set setup done");
    // // mark setup as done so mobile app commissioning flow isn't required
    // uint8_t val = 0;
    // if (!dct_read_app_data_copy(DCT_SETUP_DONE_OFFSET, &val, DCT_SETUP_DONE_SIZE) && val != 1) {
    //     val = 1;
    //     dct_write_app_data(&val, DCT_SETUP_DONE_OFFSET, DCT_SETUP_DONE_SIZE);
    // }
    // WLanCredentials creds = {};
    // creds.size = sizeof(creds);
    // creds.ssid = "Particle";
    // creds.ssid_len = strlen(creds.ssid);;
    // creds.password = "#enistek2017ParticleWifi";
    // creds.password_len = strlen(creds.password);
    // creds.security = WLAN_SEC_WPA2;
    // creds.cipher = WLAN_CIPHER_AES_TKIP;
    // network_set_credentials(NETWORK_INTERFACE_WIFI_STA, 0, &creds, NULL);
}

STARTUP(setSetupDone());

/* executes once at startup */
void setup() {
    BLE.advertise();
}

/* executes continuously after setup() runs */
void loop() {

}
