/**
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include "application.h"
#include "unit-test/unit-test.h"
#include "socket_hal.h"

#if Wiring_Cellular == 1

/* Scenario: The device will connect to the Cloud even when all
 *           TCP socket types are consumed
 *
 * Given the device is currently disconnected from the Cloud
 * When all available TCP sockets are consumed
 * And the device attempts to connect to the Cloud
 * Then the device overcomes this socket obstacle and connects to the Cloud
 */
void disconnect_from_cloud(system_tick_t timeout)
{
    Particle.disconnect();
    waitFor(Particle.disconnected, timeout);

    Cellular.disconnect();
    // Avoids some sort of race condition in AUTOMATIC mode
    delay(1000);
}
void connect_to_cloud(system_tick_t timeout)
{
    Particle.connect();
    waitFor(Particle.connected, timeout);
}
void consume_all_sockets(uint8_t protocol)
{
    static int port = 9000;
    int socket_handle;
    do {
        socket_handle = socket_create(AF_INET, SOCK_STREAM, protocol==IPPROTO_UDP ? IPPROTO_UDP : IPPROTO_TCP, port++, NIF_DEFAULT);
    } while(socket_handle_valid(socket_handle));
}
test(CELLULAR_01_device_will_connect_to_the_cloud_when_all_tcp_sockets_consumed) {
    //Serial.println("the device will connect to the cloud when all tcp sockets are consumed");
    // Given the device is currently disconnected from the Cloud
    disconnect_from_cloud(30*1000);
    // When all available TCP sockets are consumed
    consume_all_sockets(IPPROTO_TCP);
    // And the device attempts to connect to the Cloud
    connect_to_cloud(6*60*1000);
    // Then the device overcomes this socket obstacle and connects to the Cloud
    assertEqual(Particle.connected(), true);
}
/* Scenario: The device will connect to the Cloud even when all
 *           UDP socket types are consumed
 *
 * Given the device is currently disconnected from the Cloud
 * When all available UDP sockets are consumed
 * And the device attempts to connect to the Cloud
 * Then the device overcomes this socket obstacle and connects to the Cloud
 */
test(CELLULAR_02_device_will_connect_to_the_cloud_when_all_udp_sockets_consumed) {
    //Serial.println("the device will connect to the cloud when all udp sockets are consumed");
    // Given the device is currently disconnected from the Cloud
    disconnect_from_cloud(30*1000);
    // When all available UDP sockets are consumed
    consume_all_sockets(IPPROTO_UDP);
    // And the device attempts to connect to the Cloud
    connect_to_cloud(6*60*1000);
    // Then the device overcomes this socket obstacle and connects to the Cloud
    assertEqual(Particle.connected(), true);
}

test(CELLULAR_03_close_consumed_sockets) {
    for (int i = 0; i < 7; i++) {
        if (socket_handle_valid(i))
            socket_close(i);
    }
}

void checkIPAddress(const char* name, const IPAddress& address)
{
    if (address.version()==0 || address[0]==0)
    {
        Serial.print("address failed:");
        Serial.println(name);
        assertNotEqual(address.version(), 0);
        assertNotEqual(address[0], 0);
    }
}

test(CELLULAR_04_local_ip_cellular_config)
{
    connect_to_cloud(6*60*1000);
    checkIPAddress("local", Cellular.localIP());
}

test(CELLULAR_05_resolve) {
    connect_to_cloud(6*60*1000);
    checkIPAddress("www.google.com", Cellular.resolve("www.google.com"));
}

int how_many_band_options_are_available(void)
{
    CellularBand band_avail;
    if (Cellular.getBandAvailable(band_avail)) {
        return band_avail.count;
    }
    else {
        return 0;
    }
}
bool get_list_of_bands_available(CellularBand& band_avail)
{
    return Cellular.getBandAvailable(band_avail);
}
bool set_each_band_and_verify_is_selected(CellularBand& band_avail)
{
    CellularBand band_sel; // object to store bands selected
    CellularBand band_set; // object to store a single band in
    for (int x=1; x<band_avail.count; x++) { // start at first band with a frequency
        band_set.band[0] = band_avail.band[x];
        band_set.count = 1;
        if (Cellular.setBandSelect(band_set)) {
            if (Cellular.getBandSelect(band_sel)) {
                if ((band_sel.count != 1)
                    || (band_sel.band[0] != band_avail.band[x]))
                {
                    return false;
                }
            }
        }
    }
    return true;
}
bool set_each_band_as_a_string_and_verify_is_selected(CellularBand& band_avail)
{
    CellularBand band_sel; // object to store bands selected
    for (int x=1; x<band_avail.count; x++) { // start at first band with a frequency
        if (Cellular.setBandSelect(String(band_avail.band[x]).c_str())) {
            if (Cellular.getBandSelect(band_sel)) {
                if ((band_sel.count != 1)
                    || (band_sel.band[0] != band_avail.band[x]))
                {
                    return false;
                }
            }
        }
    }
    return true;
}
void get_band_select_string(CellularBand &data, char* bands, int index) {
    char band[5];
    for (int x=index; x<data.count; x++) {
        sprintf(band, "%d", data.band[x]);
        strcat(bands, band);
        if ((x+1) < data.count) strcat(bands, ",");
    }
}
bool is_bands_selected_not_equal_to_default_bands(CellularBand& band_sel, CellularBand& band_avail) {
    // create default bands string
    char band_defaults[22] = "";
    char bands_selected[22] = "";
    if (band_avail.band[0] != BAND_DEFAULT) return false;
    get_band_select_string(band_avail, band_defaults, 1);
    get_band_select_string(band_sel, bands_selected, 0);
    return (strcmp(bands_selected, band_defaults) != 0); // 0 if match
}
bool set_one_available_band(CellularBand& band_avail) {
    CellularBand band_set;
    band_set.band[0] = band_avail.band[1]; // set to first frequency available
    band_set.count = 1;
    return Cellular.setBandSelect(band_set);
}
bool is_band_available(CellularBand& band_avail, MDM_Band band)
{
    for (int x=1; x<band_avail.count; x++) { // start at first band with a frequency
        if (band_avail.band[x] == band)
            return true;
    }
    return false;
}
/* Scenario: More than 3 band options should be available from any Electron
 *
 * Given the device is currently disconnected from the Cloud
 * When we request how many bands are available
 * Then the device returns at least 3 options
 */
test(BAND_SELECT_01_more_than_three_band_options_available_on_any_electron) {
    // Given the device is currently disconnected from the Cloud
    disconnect_from_cloud(30*1000);
    // When we request how many band options are available
    int num = how_many_band_options_are_available();
    // Then the device returns at least 3 options
    assertMoreOrEqual(num, 3);
}
/* Scenario: Iterate through the available bands and check that they are set
 *
 * Given the device is currently disconnected from the Cloud
 * Given the list of available bands
 * When we iterate through the list of available bands
 * And we set and verify each one matches the original available band
 * Then all bands matched
 */
test(BAND_SELECT_02_iterate_through_the_available_bands_and_check_that_they_are_set) {
    // Given the device is currently disconnected from the Cloud
    disconnect_from_cloud(30*1000);
    // Given the list of available bands
    CellularBand band_avail;
    get_list_of_bands_available(band_avail);
    // When we iterate through the list of available bands
    // And we set and verify each one matches the original available band
    bool all_bands_matched = set_each_band_and_verify_is_selected(band_avail);
    // Then all bands matched
    assertEqual(all_bands_matched, true);
}
/* Scenario: Iterate through the available bands as strings and check that they are set
 *
 * Given the device is currently disconnected from the Cloud
 * Given the list of available bands
 * When we iterate through the list of available bands
 * And we convert each one to a string
 * And we set each one as a string
 * And we verify each one matches the original available band
 * Then all bands matched
 */
test(BAND_SELECT_03_iterate_through_the_available_bands_as_strings_and_check_that_they_are_set) {
    // Given the device is currently disconnected from the Cloud
    disconnect_from_cloud(30*1000);
    // Given the list of available bands
    CellularBand band_avail;
    get_list_of_bands_available(band_avail);
    // When we iterate through the list of available bands
    // And we convert each one to a string
    // And we set each one as a string
    // And we verify each one matches the original available band
    bool all_bands_matched = set_each_band_as_a_string_and_verify_is_selected(band_avail);
    // Then all bands matched
    assertEqual(all_bands_matched, true);
}
/* Scenario: Trying to set an invalid band will fail
 *
 * Given the device is currently disconnected from the Cloud
 * When we set an invalid band
 * Then set band select will fail
 */
test(BAND_SELECT_04_trying_to_set_an_invalid_band_will_fail) {
    // Given the device is currently disconnected from the Cloud
    disconnect_from_cloud(30*1000);
    // When we set an invalid band
    CellularBand band_set;
    band_set.band[0] = (MDM_Band)1337;
    band_set.count = 1;
    bool set_band_select_fails = Cellular.setBandSelect(band_set);
    // Then set band select will fail
    assertNotEqual(set_band_select_fails, true);
}
/* Scenario: Trying to set an invalid band as a string will fail
 *
 * Given the device is currently disconnected from the Cloud
 * When we set an invalid band as a string
 * Then set band select will fail
 */
test(BAND_SELECT_05_trying_to_set_an_invalid_band_as_a_string_will_fail) {
    // Given the device is currently disconnected from the Cloud
    disconnect_from_cloud(30*1000);
    // When we set an invalid band as a string
    bool set_band_select_fails = Cellular.setBandSelect("1337");
    // Then set band select will fail
    assertNotEqual(set_band_select_fails, true);
}
/* Scenario: Trying to set an unavailable band will fail
 *
 * Given the device is currently disconnected from the Cloud
 * Given the list of available bands
 * When we set an unavailable band
 * Then set band select will fail
 */
test(BAND_SELECT_06_trying_to_set_an_unavailable_band_will_fail) {
    // Given the device is currently disconnected from the Cloud
    disconnect_from_cloud(30*1000);
    // Given the list of available bands
    CellularBand band_avail;
    get_list_of_bands_available(band_avail);
    // When we set an unavailable band
    CellularBand band_set;
    band_set.band[0] = BAND_2600; // 4G band
    band_set.count = 1;
    bool set_band_select_fails = true;
    if (!is_band_available(band_avail, band_set.band[0])) {
        set_band_select_fails = Cellular.setBandSelect(band_set);
    }
    // Then set band select will fail
    assertNotEqual(set_band_select_fails, true);
}
/* Scenario: Setting one available band results in non default settings
 *
 * Given the device is currently disconnected from the Cloud
 * Given the list of available bands
 * When we set one available band
 * And set band select will pass
 * And get band select will pass
 * Then band select will not be default
 */
test(BAND_SELECT_07_setting_non_defaults) {
    // Given the device is currently disconnected from the Cloud
    disconnect_from_cloud(30*1000);
    // Given the list of available bands
    CellularBand band_avail;
    get_list_of_bands_available(band_avail);
    // When we set one available band
    bool set_band_select_passes = set_one_available_band(band_avail);
    // And set band select will pass
    assertEqual(set_band_select_passes, true);
    // And get band select will pass
    CellularBand band_sel;
    bool get_band_select_passes = Cellular.getBandSelect(band_sel);
    assertEqual(get_band_select_passes, true);
    // Then band select will not be default
    bool band_select_not_default = is_bands_selected_not_equal_to_default_bands(band_sel, band_avail);
    assertEqual(band_select_not_default, true);
}

/* Scenario: Setting the default band option results in default settings
 *
 * Given the device is currently disconnected from the Cloud
 * Given the list of available bands
 * When we set the default band option
 * And set band select will pass
 * And get band select will pass
 * Then band select will be default
 */
test(BAND_SELECT_08_restore_defaults) {
    // Given the device is currently disconnected from the Cloud
    disconnect_from_cloud(30*1000);
    // Given the list of available bands
    CellularBand band_avail;
    get_list_of_bands_available(band_avail);
    // When we set the default band option
    CellularBand band_set;
    band_set.band[0] = BAND_DEFAULT;
    band_set.count = 1;
    int tries = 3;
    bool set_band_select_passes = Cellular.setBandSelect(band_set);
    while (--tries > 0 && !set_band_select_passes) {
        Cellular.off();
        delay(5000);
        Cellular.on();
        delay(10000);
        // retry 3 times, important that this passes to restore defaults
        set_band_select_passes = Cellular.setBandSelect(band_set);
    }
    // And set band select will pass
    assertEqual(set_band_select_passes, true);
    // And get band select will pass
    CellularBand band_sel;
    tries = 3;
    bool get_band_select_passes = Cellular.getBandSelect(band_sel);
    while (--tries > 0 && !get_band_select_passes) {
        Cellular.off();
        delay(5000);
        Cellular.on();
        delay(10000);
        // retry 3 times, important that this passes to restore defaults
        get_band_select_passes = Cellular.setBandSelect(band_sel);
    }
    assertEqual(get_band_select_passes, true);
    // Then band select will be default
    bool band_select_default = !is_bands_selected_not_equal_to_default_bands(band_sel, band_avail);
    assertEqual(band_select_default, true);
}

#define LOREM "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Quisque ut elit nec mi bibendum mollis. Nam nec nisl mi. Donec dignissim iaculis purus, ut condimentum arcu semper quis. Phasellus efficitur ut arcu ac dignissim. In interdum sem id dictum luctus. Ut nec mattis sem. Nullam in aliquet lacus. Donec egestas nisi volutpat lobortis sodales. Aenean elementum magna ipsum, vitae pretium tellus lacinia eu. Phasellus commodo nisi at quam tincidunt, tempor gravida mauris facilisis. Duis tristique ligula ac pulvinar consectetur. Cras aliquam, leo ut eleifend molestie, arcu odio semper odio, quis sollicitudin metus libero et lorem. Donec venenatis congue commodo. Vivamus mattis elit metus, sed fringilla neque viverra eu. Phasellus leo urna, elementum vel pharetra sit amet, auctor non sapien. Phasellus at justo ac augue rutrum vulputate. In hac habitasse platea dictumst. Pellentesque nibh eros, placerat id laoreet sed, dapibus efficitur augue. Praesent pretium diam ac sem varius fermentum. Nunc suscipit dui risus sed"

test(MDM_01_socket_writes_with_length_more_than_1023_work_correctly) {
    // https://github.com/spark/firmware/issues/1104
    const char request[] =
        "POST /post HTTP/1.1\r\n"
        "Host: httpbin.org\r\n"
        "Connection: close\r\n"
        "Content-Type: multipart/form-data; boundary=-------------aaaaaaaa\r\n"
        "Content-Length: 1124\r\n"
        "\r\n"
        "---------------aaaaaaaa\r\n"
        "Content-Disposition: form-data; name=\"field\"\r\n"
        "\r\n"
        LOREM "\r\n"
        "---------------aaaaaaaa--\r\n";
    const int requestSize = sizeof(request) - 1;

    Cellular.connect();
    waitFor(Cellular.ready, 120000);

    TCPClient c;
    int res = c.connect("httpbin.org", 80);
    (void)res;

    int sz = c.write((const uint8_t*)request, requestSize);
    assertEqual(sz, requestSize);

    char* responseBuf = new char[2048];
    memset(responseBuf, 0, 2048);
    int responseSize = 0;
    uint32_t mil = millis();
    while(1) {
        while (c.available()) {
            responseBuf[responseSize++] = c.read();
        }
        if (!c.connected())
            break;
        if (millis() - mil >= 60000) {
            break;
        }
    }

    bool contains = false;
    if (responseSize > 0 && !c.connected()) {
        contains = strstr(responseBuf, LOREM) != nullptr;
    }

    delete responseBuf;

    assertTrue(contains);
}

static int atCallback(int type, const char* buf, int len, int* lines) {
    if (len && type == TYPE_UNKNOWN)
        (*lines)++;
    return WAIT;
}

test(MDM_02_at_commands_with_long_response_are_correctly_parsed_and_flow_controlled) {
    // https://github.com/spark/firmware/issues/1138
    int lines = 0;
    int ret = -99999;
    // Disconnected from the Cloud so we are not dealing with any other command responses
    Particle.disconnect();
    waitFor(Particle.disconnected, 30000);

    while ((ret = Cellular.command(atCallback, &lines, 10000, "AT+CLAC\r\n")) == WAIT);
    assertEqual(ret, (int)RESP_OK);
    assertMoreOrEqual(lines, 200);
}

#endif
