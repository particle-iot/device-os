#include "Particle.h"

#if (Wiring_Cellular == 1) && !HAL_PLATFORM_NCP

bool passing = true;
#define CHECK_SUCCESS(x) { if (!(x)) { passing = false; break; } }

SerialLogHandler logHandler(115200, LOG_LEVEL_NONE, {
    { "app", LOG_LEVEL_ALL }, // enable all app messages
    // { "system", LOG_LEVEL_INFO } // only info level for system messages
});

// Enable threading if compiled with "USE_THREADING=y"
#if PLATFORM_THREADING == 1 && USE_THREADING == 1
SYSTEM_THREAD(ENABLED);
#endif

/**
 * Returns current modem type:
 * DEV_UNKNOWN, DEV_SARA_G350, DEV_SARA_U260, DEV_SARA_U270, DEV_SARA_U201, DEV_SARA_R410
 */
int cellular_modem_type() {
    CellularDevice device;
    memset(&device, 0, sizeof(device));
    device.size = sizeof(device);
    cellular_device_info(&device, NULL);

    return device.dev;
}

/* Scenario: The device will connect to the Cloud even when all
 *           TCP socket types are consumed
 *
 * Given the device is currently disconnected from the Cloud
 * When all available TCP sockets are consumed
 * And the device attempts to connect to the Cloud
 * Then the device overcomes this socket obstacle and connects to the Cloud
 */
void disconnect_from_cloud(system_tick_t timeout, bool detach = false)
{
    Particle.disconnect();
    waitFor(Particle.disconnected, timeout);

    Cellular.disconnect();
    // Avoids some sort of race condition in AUTOMATIC mode
    delay(1000);

    if (detach) {
        Cellular.command(timeout, "AT+COPS=2,2\r\n");
    }
}
void connect_to_cloud(system_tick_t timeout)
{
    Particle.connect();
    waitFor(Particle.connected, timeout);
}
int how_many_band_options_are_available(void)
{
    CellularBand band_avail;
    if (Cellular.getBandAvailable(band_avail)) {
        return band_avail.count;
    }
    else {
        return true;
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
bool skip_r410 = false;
/* Scenario: More than 3 band options should be available from any Electron
 *
 * Given the device is currently disconnected from the Cloud
 * When we request how many bands are available
 * Then the device returns at least 3 options
 */
bool BAND_SELECT_01_more_than_three_band_options_available_on_any_electron() {
    if (cellular_modem_type() == DEV_SARA_R410) {
        skip_r410 = true;
        Serial.println("Skipping tests on R410!");
        return true;
    }
    Log.trace("%s", __func__);
    // Given the device is currently disconnected from the Cloud
    disconnect_from_cloud(30*1000, true);
    // When we request how many band options are available
    int num = how_many_band_options_are_available();
    // Then the device returns at least 3 options
    if (num < 3) {
        Log.error("(%d<3) : num < 3", num);
        return false;
    }
    return true;
}
/* Scenario: Iterate through the available bands and check that they are set
 *
 * Given the device is currently disconnected from the Cloud
 * Given the list of available bands
 * When we iterate through the list of available bands
 * And we set and verify each one matches the original available band
 * Then all bands matched
 */
bool BAND_SELECT_02_iterate_through_the_available_bands_and_check_that_they_are_set() {
    if (skip_r410) {
        return true;
    }
    Log.trace("%s", __func__);
    // Given the device is currently disconnected from the Cloud
    disconnect_from_cloud(30*1000, true);
    // Given the list of available bands
    CellularBand band_avail;
    get_list_of_bands_available(band_avail);
    // When we iterate through the list of available bands
    // And we set and verify each one matches the original available band
    bool all_bands_matched = set_each_band_and_verify_is_selected(band_avail);
    // Then all bands matched
    if (all_bands_matched != true) {
        Log.error("all_bands_matched != true");
        return false;
    }
    return true;
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
bool BAND_SELECT_03_iterate_through_the_available_bands_as_strings_and_check_that_they_are_set() {
    if (skip_r410) {
        return true;
    }
    Log.trace("%s", __func__);
    // Given the device is currently disconnected from the Cloud
    disconnect_from_cloud(30*1000, true);
    // Given the list of available bands
    CellularBand band_avail;
    get_list_of_bands_available(band_avail);
    // When we iterate through the list of available bands
    // And we convert each one to a string
    // And we set each one as a string
    // And we verify each one matches the original available band
    bool all_bands_matched = set_each_band_as_a_string_and_verify_is_selected(band_avail);
    // Then all bands matched
    if (all_bands_matched != true) {
        Log.error("all_bands_matched != true");
        return false;
    }
    return true;
}
/* Scenario: Trying to set an invalid band will fail
 *
 * Given the device is currently disconnected from the Cloud
 * When we set an invalid band
 * Then set band select will fail
 */
bool BAND_SELECT_04_trying_to_set_an_invalid_band_will_fail() {
    if (skip_r410) {
        return true;
    }
    Log.trace("%s", __func__);
    // Given the device is currently disconnected from the Cloud
    disconnect_from_cloud(30*1000, true);
    // When we set an invalid band
    CellularBand band_set;
    band_set.band[0] = (MDM_Band)1337;
    band_set.count = 1;
    bool set_band_select_fails = Cellular.setBandSelect(band_set);
    // Then set band select will fail
    if (set_band_select_fails == true) {
        Log.error("set_band_select_fails == true");
        return false;
    }
    return true;
}
/* Scenario: Trying to set an invalid band as a string will fail
 *
 * Given the device is currently disconnected from the Cloud
 * When we set an invalid band as a string
 * Then set band select will fail
 */
bool BAND_SELECT_05_trying_to_set_an_invalid_band_as_a_string_will_fail() {
    if (skip_r410) {
        return true;
    }
    Log.trace("%s", __func__);
    // Given the device is currently disconnected from the Cloud
    disconnect_from_cloud(30*1000, true);
    // When we set an invalid band as a string
    bool set_band_select_fails = Cellular.setBandSelect("1337");
    // Then set band select will fail
    if (set_band_select_fails == true) {
        Log.error("set_band_select_fails == true");
        return false;
    }
    return true;
}
/* Scenario: Trying to set an unavailable band will fail
 *
 * Given the device is currently disconnected from the Cloud
 * Given the list of available bands
 * When we set an unavailable band
 * Then set band select will fail
 */
bool BAND_SELECT_06_trying_to_set_an_unavailable_band_will_fail() {
    if (skip_r410) {
        return true;
    }
    Log.trace("%s", __func__);
    // Given the device is currently disconnected from the Cloud
    disconnect_from_cloud(30*1000, true);
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
    if (set_band_select_fails == true) {
        Log.error("set_band_select_fails == true");
        return false;
    }
    return true;
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
bool BAND_SELECT_07_setting_non_defaults() {
    if (skip_r410) {
        return true;
    }
    Log.trace("%s", __func__);
    // Given the device is currently disconnected from the Cloud
    disconnect_from_cloud(30*1000, true);
    // Given the list of available bands
    CellularBand band_avail;
    get_list_of_bands_available(band_avail);
    // When we set one available band
    bool set_band_select_passes = set_one_available_band(band_avail);
    // And set band select will pass
    if (set_band_select_passes != true) {
        Log.error("set_band_select_passes != true");
        return false;
    }
    // And get band select will pass
    CellularBand band_sel;
    bool get_band_select_passes = Cellular.getBandSelect(band_sel);
    if (get_band_select_passes != true) {
        Log.error("get_band_select_passes != true");
        return false;
    }
    // Then band select will not be default
    bool band_select_not_default = is_bands_selected_not_equal_to_default_bands(band_sel, band_avail);
    if (band_select_not_default != true) {
        Log.error("band_select_not_default != true");
        return false;
    }
    return true;
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
bool BAND_SELECT_08_restore_defaults() {
    if (skip_r410) {
        return true;
    }
    Log.trace("%s", __func__);
    // Given the device is currently disconnected from the Cloud
    disconnect_from_cloud(30*1000, true);
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
        cellular_off(NULL);  // blocking call to avoid adding delays
        cellular_on(NULL);  // blocking call to avoid adding delays
        // Given the device is currently disconnected from the Cloud
        disconnect_from_cloud(30*1000, true);
        // retry 3 times, important that this passes to restore defaults
        set_band_select_passes = Cellular.setBandSelect(band_set);
    }
    // And set band select will pass
    if (set_band_select_passes != true) {
        Log.error("set_band_select_passes != true");
        return false;
    }
    // And get band select will pass
    CellularBand band_sel;
    tries = 3;
    // Given the device is currently disconnected from the Cloud
    disconnect_from_cloud(30*1000, true);
    bool get_band_select_passes = Cellular.getBandSelect(band_sel);
    while (--tries > 0 && !get_band_select_passes) {
        cellular_off(NULL);  // blocking call to avoid adding delays
        cellular_on(NULL);  // blocking call to avoid adding delays
        // Given the device is currently disconnected from the Cloud
        disconnect_from_cloud(30*1000, true);
        // retry 3 times, important that this passes to restore defaults
        get_band_select_passes = Cellular.setBandSelect(band_sel);
    }
    if (get_band_select_passes != true) {
        Log.error("get_band_select_passes != true");
        return false;
    }
    // Then band select will be default
    bool band_select_default = !is_bands_selected_not_equal_to_default_bands(band_sel, band_avail);
    if (band_select_default != true) {
        Log.error("band_select_default != true");
        return false;
    }
    return true;
}

bool BAND_SELECT_09_restore_connection() {
    if (skip_r410) {
        return true;
    }
    Log.trace("%s", __func__);
    // Allow network registration
    cellular_command(NULL, NULL, 30000, "AT+COPS=0,2\r\n"); // blocking call to avoid adding delays
    connect_to_cloud(6*60*1000);
    if (Particle.disconnected()) {
        Log.error("Connection was not restored and timed out!");
        return false;
    }
    return true;
}

void setup() {
    while (!Serial.isConnected()) {
        Particle.process();
    }

    do {
    CHECK_SUCCESS( BAND_SELECT_01_more_than_three_band_options_available_on_any_electron() )
    CHECK_SUCCESS( BAND_SELECT_02_iterate_through_the_available_bands_and_check_that_they_are_set() )
    CHECK_SUCCESS( BAND_SELECT_03_iterate_through_the_available_bands_as_strings_and_check_that_they_are_set() )
    CHECK_SUCCESS( BAND_SELECT_04_trying_to_set_an_invalid_band_will_fail() )
    CHECK_SUCCESS( BAND_SELECT_05_trying_to_set_an_invalid_band_as_a_string_will_fail())
    CHECK_SUCCESS( BAND_SELECT_06_trying_to_set_an_unavailable_band_will_fail() )
    CHECK_SUCCESS( BAND_SELECT_07_setting_non_defaults() )
    CHECK_SUCCESS( BAND_SELECT_08_restore_defaults() )
    CHECK_SUCCESS( BAND_SELECT_09_restore_connection() )
    } while (false);

    RGB.control(true);
    if (passing) {
        RGB.color(0,255,0); // GREEN
        Serial.println("\r\n\r\nAll tests passed!");
    } else {
        RGB.color(255,0,0); // RED
        Serial.println("\r\n\r\nTests failed!!!");
    }
}

#else
#error "Tests are currently only for Gen 2 Cellular devices"
#endif // #if (Wiring_Cellular == 1) && !HAL_PLATFORM_NCP

