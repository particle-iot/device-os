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
test(device_will_connect_to_the_cloud_when_all_tcp_sockets_consumed) {
    //Serial.println("the device will connect to the cloud when all tcp sockets are consumed");
    // Given the device is currently disconnected from the Cloud
    disconnect_from_cloud(30*1000);
    // When all available TCP sockets are consumed
    consume_all_sockets(IPPROTO_TCP);
    // And the device attempts to connect to the Cloud
    Particle.connect();
    // Then the device overcomes this socket obstacle and connects to the Cloud
    connect_to_cloud(60*1000);
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
test(device_will_connect_to_the_cloud_when_all_udp_sockets_consumed) {
    //Serial.println("the device will connect to the cloud when all udp sockets are consumed");
    // Given the device is currently disconnected from the Cloud
    disconnect_from_cloud(30*1000);
    // When all available UDP sockets are consumed
    consume_all_sockets(IPPROTO_UDP);
    // And the device attempts to connect to the Cloud
    Particle.connect();
    // Then the device overcomes this socket obstacle and connects to the Cloud
    connect_to_cloud(60*1000);
    assertEqual(Particle.connected(), true);
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
test(BAND_SELECT_more_than_three_band_options_available_on_any_electron) {
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
test(BAND_SELECT_iterate_through_the_available_bands_and_check_that_they_are_set) {
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
test(BAND_SELECT_iterate_through_the_available_bands_as_strings_and_check_that_they_are_set) {
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
test(BAND_SELECT_trying_to_set_an_invalid_band_will_fail) {
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
test(BAND_SELECT_trying_to_set_an_invalid_band_as_a_string_will_fail) {
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
test(BAND_SELECT_trying_to_set_an_unavailable_band_will_fail) {
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
test(BAND_SELECT_setting_one_available_band_results_in_non_defaults) {
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
/* Scenario: Setting one available band results in non default settings
 *           and setting the default band option results in default settings
 *
 * Given the device is currently disconnected from the Cloud
 * Given the list of available bands
 * When we set one available band
 * And set band select will pass
 * And get band select will pass
 * Then band select will not be default
 * When we set the default band option
 * And set band select will pass
 * And get band select will pass
 * Then band select will be default
 */
test(BAND_SELECT_setting_non_defaults_then_restore_defaults) {
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
    // When we set the default band option
    CellularBand band_set;
    band_set.band[0] = BAND_DEFAULT;
    band_set.count = 1;
    set_band_select_passes = Cellular.setBandSelect(band_set);
    // And set band select will pass
    assertEqual(set_band_select_passes, true);
    // And get band select will pass
    get_band_select_passes = Cellular.getBandSelect(band_sel);
    assertEqual(get_band_select_passes, true);
    // Then band select will be default
    bool band_select_default = !is_bands_selected_not_equal_to_default_bands(band_sel, band_avail);
    assertEqual(band_select_default, true);
}
#endif

#if Wiring_Cellular == 1

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

test(cellular_config)
{
	checkIPAddress("local", Cellular.localIP());
}

#endif
